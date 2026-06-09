#include "Characters/Component/GeoCharacterMovementComponent.h"

#include "Characters/GeoCharacter.h"

UGeoCharacterMovementComponent::UGeoCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // movement driven via ProcessInput calls
}

void UGeoCharacterMovementComponent::OnRegister()
{
	Super::OnRegister();
	// Cache the designer-configured base here, not in BeginPlay: on a listen-server host the pawn is possessed and its
	// MovementSpeedMultiplier attribute is applied (firing ApplySpeedMultiplier) BEFORE BeginPlay runs. Caching in
	// BeginPlay would then snapshot the already-multiplied (or zeroed) value. OnRegister runs before possession on
	// every net mode, so the cached base is always the real default.
	BaseMaxWalkSpeed = MaxWalkSpeed;
	BaseMaxAcceleration = MaxAcceleration;
}

void UGeoCharacterMovementComponent::ApplySpeedMultiplier(float Multiplier)
{
	MaxWalkSpeed = BaseMaxWalkSpeed * Multiplier;
	MaxAcceleration = BaseMaxAcceleration * Multiplier;
}

AGeoCharacter* UGeoCharacterMovementComponent::GetGeoCharacter() const
{
	return Cast<AGeoCharacter>(GetOwner());
}
