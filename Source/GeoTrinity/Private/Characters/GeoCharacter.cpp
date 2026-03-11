#include "Characters/GeoCharacter.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GeoMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "HUD/Component/GeoCombattantWidgetComp.h"
#include "Input/GeoInputComponent.h"
#include "Tool/UGameplayLibrary.h"
#include "VisualLogger/VisualLogger.h"

static TAutoConsoleVariable CVarShowCharacterServerLocation(
	TEXT("Geo.ShowCharacterServerLocation"), false,
	TEXT("When true, the character server location will appear as draw sphere with simple collision size"));

// Sets default values
AGeoCharacter::AGeoCharacter(FObjectInitializer const& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UGeoMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetIsReplicated(true);
	MeshComp->SetCastShadow(false);
	// Set default collision profiles
	MeshComp->SetCollisionProfileName(TEXT("GeoShape"));

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("GeoCapsule"));
	GetCapsuleComponent()->SetCapsuleHalfHeight(ArbitraryCharacterZ);
	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoInputComponent->SetIsReplicated(true);

	CharacterWidgetComponent = CreateDefaultSubobject<UGeoCombattantWidgetComp>(TEXT("CharacterWidgetComponent"));
	CharacterWidgetComponent->SetupAttachment(GetRootComponent());
	CharacterWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);

	bUseControllerRotationYaw = true;

	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AGeoCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UE_VLOG_LOCATION(this, LogGeoTrinity, Verbose, GetActorLocation(), 30.f, UGameplayLibrary::GetColorForObject(this),
					 TEXT("%s [%s]"), *GetName(), *UEnum::GetValueAsString(GetLocalRole()));

	if (CVarShowCharacterServerLocation.GetValueOnGameThread() && UGameplayLibrary::IsServer(GetWorld()))
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), GetSimpleCollisionRadius(), 8,
						UGameplayLibrary::GetColorForObject(GetOuter()), false, 0.f);
	}
}

UAbilitySystemComponent* AGeoCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGeoCharacter::DrawDebugVectorFromCharacter(FVector const& Direction, FString const& DebugMessage) const
{
	DrawDebugVectorFromCharacter(Direction, DebugMessage, UGameplayLibrary::GetColorForObject(this));
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
}

#ifdef UE_EDITOR
void AGeoCharacter::BeginPlay()
{
	Super::BeginPlay();
	LocalRoleForDebugPurpose = GetLocalRole();
}
#endif
