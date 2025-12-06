#include "Characters/GeoCharacter.h"

#include "AbilitySystem/InteractableComponent.h"
#include "Components/CapsuleComponent.h"
#include "GeoInputComponent.h"
#include "GeoMovementComponent.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "GeoTrinity/GeoTrinity.h"

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

FGenericTeamId AGeoCharacter::GetGenericTeamId() const
{
	return FGenericTeamId(static_cast<uint8>(TeamId));
}

FColor AGeoCharacter::GetColorForCharacter(const AGeoCharacter* Character)
{
	if (!IsValid(Character))
	{
		return FColor::White;
	}

	static const FColor Palette[] = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Cyan,
		FColor::Magenta, FColor::Orange, FColor::Emerald, FColor::Purple, FColor::Turquoise, FColor::Silver};

	return Palette[Character->GetUniqueID() % std::size(Palette)];
}

// void AGeoCharacter::VLogBoxes(const FInputStep& InputStep, const FColor Color) const
// {
// 	UE_VLOG_BOX(this, LogGeoTrinity, VeryVerbose,
// 		FBox(FVector(GetBox().Min, 0.f) + GetActorLocation(), FVector(GetBox().Max, 0.f) + GetActorLocation()), Color,
// 		TEXT("LocalTime %s, delta time %.5f"), *InputStep.Time.ToString(), InputStep.DeltaTimeSeconds);
// }

void AGeoCharacter::DrawDebugVectorFromCharacter(const FVector& Direction, const FString& DebugMessage) const
{
	DrawDebugVectorFromCharacter(Direction, DebugMessage, GetColorForCharacter(this));
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

		// Single-frame arrow (non-persistent) so it updates every tick without clutter
		DrawDebugDirectionalArrow(World, Start, End, 20.f, Color,
			/*bPersistentLines*/ false,
			/*LifeTime*/ 0.f, /*DepthPriority*/ 0, /*Thickness*/ 2.f);

		UE_VLOG_ARROW(this, LogGeoTrinity, VeryVerbose, Start, End, Color, TEXT("%s"), *DebugMessage);
	}
}
