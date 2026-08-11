#include "Characters/GeoCharacter.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Interface/GeoCombattantWidgetHost.h"
#include "Input/GeoInputComponent.h"
#include "Net/UnrealNetwork.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "VisualLogger/VisualLogger.h"

static TAutoConsoleVariable<bool> CVarPlayerInvincible(TEXT("Geo.PlayerInvincible"), false,
													   TEXT("When true, players never die (zero health or falling)."),
													   ECVF_Cheat);

static TAutoConsoleVariable CVarShowCharacterServerLocation(
	TEXT("Geo.ShowCharacterServerLocation"), false,
	TEXT("When true, the character server location will appear as draw sphere with simple collision size"));

// Sets default values
AGeoCharacter::AGeoCharacter(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UGeoCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetIsReplicated(true);
	MeshComp->SetCastShadow(false);
	// Set default collision profiles
	MeshComp->SetCollisionProfileName(TEXT("GeoShape"));

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("GeoCapsule"));
	GetCapsuleComponent()->SetCapsuleHalfHeight(ArbitraryCharacterZ);
	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoInputComponent->SetIsReplicated(true);

	WidgetAnchorComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WidgetAnchorComponent"));
	WidgetAnchorComponent->SetupAttachment(GetRootComponent());
	WidgetAnchorComponent->SetUsingAbsoluteRotation(true);

	// Concrete class comes from settings (a soft path) so gameplay never names the UI-module UGeoCombattantWidgetComp.
	// Optional subobject: the dedicated-server target doesn't ship the UI class, so it resolves to null and is skipped.
	if (UClass* const WidgetComponentClass =
			GetDefault<UGameDataSettings>()->CombattantWidgetComponentClass.LoadSynchronous())
	{
		CharacterWidgetComponent = Cast<UWidgetComponent>(ObjectInitializer.CreateDefaultSubobject(
			this, TEXT("CharacterWidgetComponent"), UWidgetComponent::StaticClass(), WidgetComponentClass,
			/*bIsRequired=*/false, /*bIsTransient=*/false));
		CharacterWidgetComponent->SetupAttachment(WidgetAnchorComponent);
		// Orthographic top-down: World space lies the bar flat on the ground; Screen space at desired size keeps it
		// upright and sized to the WBP. Both stay BP-overridable.
		CharacterWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		CharacterWidgetComponent->SetDrawAtDesiredSize(true);
		if (UClass* const WidgetClass =
				GetDefault<UGameDataSettings>()->DefaultCharacterHealthBarWidgetClass.LoadSynchronous())
		{
			CharacterWidgetComponent->SetWidgetClass(WidgetClass);
		}
	}

	GameFeelComponent = CreateDefaultSubobject<UGeoGameFeelComponent>(TEXT("GameFeelComponent"));

	DeployableManagerComponent =
		CreateDefaultSubobject<UGeoDeployableManagerComponent>(TEXT("DeployableManagerComponent"));

	bUseControllerRotationYaw = true;

	GetCharacterMovement()->bOrientRotationToMovement = false;
}


void AGeoCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGeoCharacter, bIsDead);
	DOREPLIFETIME(AGeoCharacter, bDiedFromFall);
}

void AGeoCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UE_VLOG_LOCATION(this, LogGeoTrinity, Verbose, GetActorLocation(), 30.f, GeoLib::GetColorForObject(this),
					 TEXT("%s [%s]"), *GetName(), *UEnum::GetValueAsString(GetLocalRole()));

	if (CVarShowCharacterServerLocation.GetValueOnGameThread() && GeoLib::IsServer(GetWorld()))
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), GetSimpleCollisionRadius(), 8,
						GeoLib::GetColorForObject(GetOuter()), false, 0.f);
	}

	if (Controller && !bIsDead)
	{
		float const CurrentYaw = Controller->GetControlRotation().Yaw;
		float const DeltaAngle = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
		float const MaxDelta = MaxRotationSpeed * DeltaSeconds;
		float const ClampedDelta = FMath::Clamp(DeltaAngle, -MaxDelta, MaxDelta);
		Controller->SetControlRotation(FRotator(0.f, CurrentYaw + ClampedDelta, 0.f));
	}
}

void AGeoCharacter::StopAllSpawnedElements()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->StopAllActivePatterns();
	}
	if (DeployableManagerComponent)
	{
		DeployableManagerComponent->ForceExpireAll();
	}
}
void AGeoCharacter::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	StopAllSpawnedElements();
	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* AGeoCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGeoCharacter::SetCombattantWidgetVisible(bool const bVisible)
{
	if (CharacterWidgetComponent)
	{
		CharacterWidgetComponent->SetHiddenInGame(!bVisible);
	}
}

void AGeoCharacter::DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage) const
{
	DrawDebugVectorFromCharacter(Direction, DebugMessage, GeoLib::GetColorForObject(this));
}

void AGeoCharacter::DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage,
												 FColor Color) const
{
	// Debug: draw a world-space line (arrow) from the character showing the look vector
	if (UWorld* World = GetWorld())
	{
		FVector Start = GetActorLocation();
		Start.Z = 0.f;
		FVector const Dir = Direction.GetSafeNormal();
		constexpr float Length = 500.f; // visualized length of the vector
		FVector const End = Start + Dir * Length;

		DrawDebugDirectionalArrow(World, Start, End, 20.f, Color, false, 0.f, 0, 2.f);

		UE_VLOG_ARROW(this, LogGeoTrinity, VeryVerbose, Start, End, Color, TEXT("%s"), *DebugMessage);
	}
}

void AGeoCharacter::InitGAS()
{
	AbilitySystemComponent->InitializeDefaultAttributes();
	if (GeoLib::IsServer(this))
	{
		AbilitySystemComponent->GiveStartupAbilities();
	}

	BindCombattantWidgetToASC();
}

void AGeoCharacter::BindCombattantWidgetToASC()
{
	// Called from every point the ASC or its attributes can become available, because none of them alone covers both
	// roles: the host sets attributes synchronously with no replication callback to bind on, while a remote proxy only
	// receives its ASC with OnRep_PlayerState — after the bar's first bind. Miss one and the bar reads MaxHealth as 0
	// and collapses. BindToOwnerASC is idempotent, so binding again costs nothing.
	if (IGeoCombattantWidgetHost* WidgetHost = Cast<IGeoCombattantWidgetHost>(CharacterWidgetComponent))
	{
		WidgetHost->BindToOwnerASC();
	}
}

void AGeoCharacter::BeginPlay()
{
	Super::BeginPlay();
	TargetYaw = GetActorRotation().Yaw;
	ensureMsgf(CharacterWidgetComponent || GeoLib::IsDedicatedServer(GetWorld()),
			   TEXT("%s has no CharacterWidgetComponent — set CombattantWidgetComponentClass in Game Data Settings"),
			   *GetName());
	BindCombattantWidgetToASC();

	// A screen-space UWidgetComponent draws in the game layer of its pawn's own local player, and couch coop forces
	// splitscreen off, which leaves every local player above 0 with a zero-sized view: their layer is clipped away and
	// ULocalPlayer::GetProjectionData refuses to project into it, so player 2's bars would never appear. Point them all
	// at the player that owns the one shared view.
	if (ULocalPlayer* const SharedViewPlayer = GetGameInstance()->GetFirstGamePlayer())
	{
		TInlineComponentArray<UWidgetComponent*> const WidgetComponents(this);
		for (UWidgetComponent* const WidgetComponent : WidgetComponents)
		{
			WidgetComponent->SetOwnerPlayer(SharedViewPlayer);
		}
	}
#if WITH_EDITOR
	LocalRoleForDebugPurpose = GetLocalRole();
#endif
}


void AGeoCharacter::Death(bool const bFromFall)
{
	if (bIsDead || CVarPlayerInvincible.GetValueOnGameThread())
	{
		return;
	}
	bDiedFromFall = bFromFall;
	bIsDead = true;
	DeathLogic();
}

void AGeoCharacter::DeathLogic()
{
	Destroy();
}

void AGeoCharacter::ReviveLogic()
{
	// does nothing by default
}

void AGeoCharacter::SetDeathVisuals(bool const bDead)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	UAnimMontage* Montage = GetDeathMontage();
	if (!AnimInstance || !Montage)
	{
		return;
	}

	if (bDead)
	{
		AnimInstance->Montage_Play(Montage);
	}
	else
	{
		AnimInstance->Montage_Stop(Montage->GetDefaultBlendOutTime(), Montage);
	}
}

void AGeoCharacter::OnRep_IsDead(bool const bOldValue)
{
	if (bIsDead && !bOldValue)
	{
		DeathLogic();
	}
	else if (!bIsDead && bOldValue)
	{
		HandleRevived();
	}
}


void AGeoCharacter::Revive()
{
	if (!bIsDead)
	{
		return;
	}
	bIsDead = false;
	HandleRevived();
}

void AGeoCharacter::HandleRevived()
{
	ReviveLogic();
	OnRevived.Broadcast();
}
