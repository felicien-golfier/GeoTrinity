#include "Characters/GeoCharacter.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GeoMovementComponent.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Input/GeoInputComponent.h"
#include "Tool/GameplayLibrary.h"

// Sets default values
AGeoCharacter::AGeoCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(
		  ObjectInitializer.SetDefaultSubobjectClass<UGeoMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh Component"));
	MeshComponent->SetIsReplicated(true);
	MeshComponent->SetupAttachment(GetCapsuleComponent());
	MeshComponent->SetCastShadow(false);

	// Set default collision profiles
	MeshComponent->SetCollisionProfileName(TEXT("GeoShape"));
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("GeoCapsule"));

	GeoInputComponent = CreateDefaultSubobject<UGeoInputComponent>(TEXT("Geo Input Component"));
	GeoInputComponent->SetIsReplicated(true);

	// Disable orient-to-movement; we will rotate manually toward aim
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
}

UAbilitySystemComponent* AGeoCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

ETeamAttitude::Type AGeoCharacter::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
	if (!OtherTeamAgent)
	{
		return ETeamAttitude::Neutral;
	}

	return OtherTeamAgent->GetGenericTeamId() == GetGenericTeamId() ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

void AGeoCharacter::DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage) const
{
	DrawDebugVectorFromCharacter(Direction, DebugMessage, GameplayLibrary::GetColorForObject(this));
}

void AGeoCharacter::DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage,
	FColor Color) const
{
	// Debug: draw a world-space line (arrow) from the character showing the look vector
	if (UWorld* World = GetWorld())
	{
		const FVector Start = GetActorLocation();
		const FVector Dir = Direction.GetSafeNormal();
		constexpr float Length = 500.f;   // visualized length of the vector
		const FVector End = Start + Dir * Length;

		DrawDebugDirectionalArrow(World, Start, End, 20.f, Color, false, 0.f, 0, 2.f);

		UE_VLOG_ARROW(this, LogGeoTrinity, VeryVerbose, Start, End, Color, TEXT("%s"), *DebugMessage);
	}
}

void AGeoCharacter::InitGAS()
{
	AbilitySystemComponent->InitializeDefaultAttributes();
	AbilitySystemComponent->GiveStartupAbilities();
}

#ifdef UE_EDITOR
void AGeoCharacter::BeginPlay()
{
	Super::BeginPlay();
	LocalRoleForDebugPurpose = GetLocalRole();
}
#endif