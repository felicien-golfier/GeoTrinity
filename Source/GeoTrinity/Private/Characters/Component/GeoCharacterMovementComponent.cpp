#include "Characters/Component/GeoCharacterMovementComponent.h"

#include "Characters/GeoCharacter.h"

UGeoCharacterMovementComponent::UGeoCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // movement driven via ProcessInput calls
}

void UGeoCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
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
