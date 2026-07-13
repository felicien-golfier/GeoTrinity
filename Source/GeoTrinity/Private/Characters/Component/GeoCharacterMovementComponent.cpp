#include "Characters/Component/GeoCharacterMovementComponent.h"

#include "Characters/GeoCharacter.h"

UGeoCharacterMovementComponent::UGeoCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // movement driven via ProcessInput calls

	// Corrections are eased into the mesh as a world-space offset; the default 0.1s blend reads as a visible
	// wobble at 50+ ping. Blend faster and snap past capsule-sized errors instead of gliding through them.
	NetworkSimulatedSmoothLocationTime = 0.05f;
	NetworkMaxSmoothUpdateDistance = 140.f;
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
