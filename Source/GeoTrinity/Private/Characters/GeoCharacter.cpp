#include "Characters/GeoCharacter.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Interface/GeoCombattantWidgetHost.h"
#include "Input/GeoInputComponent.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGeoGameplayLibrary.h"
#include "VisualLogger/VisualLogger.h"

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
}

void AGeoCharacter::StopAllSpawnedElements()
{
	AbilitySystemComponent->StopAllActivePatterns();
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
	if (HasAuthority())
	{
		AbilitySystemComponent->GiveStartupAbilities();
	}

	// The host sets attributes synchronously (no replication callback), so re-bind now that they exist or the bar reads
	// MaxHealth as 0 and collapses.
	if (IGeoCombattantWidgetHost* WidgetHost = Cast<IGeoCombattantWidgetHost>(CharacterWidgetComponent))
	{
		WidgetHost->BindToOwnerASC();
	}
}

void AGeoCharacter::BeginPlay()
{
	Super::BeginPlay();
	ensureMsgf(CharacterWidgetComponent || GeoLib::IsDedicatedServer(GetWorld()),
			   TEXT("%s has no CharacterWidgetComponent — set CombattantWidgetComponentClass in Game Data Settings"),
			   *GetName());
	// The host sets attributes synchronously (no replication callback to bind the bar), so bind here once they exist.
	if (IGeoCombattantWidgetHost* WidgetHost = Cast<IGeoCombattantWidgetHost>(CharacterWidgetComponent))
	{
		WidgetHost->BindToOwnerASC();
	}
	// Hide the floating bar only over the local player's own avatar — it uses the main HUD overlay instead.
	SetCombattantWidgetVisible(!GeoLib::IsLocalPlayerAvatar(this));
#ifdef UE_EDITOR
	LocalRoleForDebugPurpose = GetLocalRole();
#endif
}
