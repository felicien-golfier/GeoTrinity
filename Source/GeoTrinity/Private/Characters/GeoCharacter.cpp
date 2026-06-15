#include "Characters/GeoCharacter.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Characters/Component/GeoCharacterMovementComponent.h"
#include "Characters/Component/GeoDeployableManagerComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Components/CapsuleComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Component/GeoCombattantWidgetComp.h"
#include "Input/GeoInputComponent.h"
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

	CharacterWidgetComponent = CreateDefaultSubobject<UGeoCombattantWidgetComp>(TEXT("CharacterWidgetComponent"));
	CharacterWidgetComponent->SetupAttachment(WidgetAnchorComponent);
	CharacterWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	CharacterWidgetComponent->SetDrawAtDesiredSize(true);
	CharacterWidgetComponent->SetRelativeLocation(FVector(100.f, 0.f, 0.f));

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

	// The floating bar's component binds in BeginPlay, before attributes exist, so it collapses reading MaxHealth as 0.
	// On clients replication later fires the change delegate and re-shows it, but the listen-server host sets
	// attributes synchronously with no replication callback — so (re)bind now that attributes are initialized.
	// (APlayableCharacter overrides InitGAS and calls this itself; InitializeForOwner is idempotent.)
	CharacterWidgetComponent->BindWidgetToOwnerASC();
}

void AGeoCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Don't draw a floating bar over our own avatar — the local player uses the main HUD overlay.
	// Enemies are never locally controlled, so this is a no-op for them.
	SetCombattantWidgetVisible(!IsLocallyControlled());
#ifdef UE_EDITOR
	LocalRoleForDebugPurpose = GetLocalRole();
#endif
}
