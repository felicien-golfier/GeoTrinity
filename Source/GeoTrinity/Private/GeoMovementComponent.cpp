#include "GeoMovementComponent.h"

#include "Characters/GeoCharacter.h"

UGeoMovementComponent::UGeoMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // movement driven via ProcessInput calls
}

void UGeoMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	BaseMaxWalkSpeed = MaxWalkSpeed;
}

void UGeoMovementComponent::ApplySpeedMultiplier(float Multiplier)
{
	MaxWalkSpeed = BaseMaxWalkSpeed * Multiplier;
}

AGeoCharacter* UGeoMovementComponent::GetGeoCharacter() const
{
	return Cast<AGeoCharacter>(GetOwner());
}
