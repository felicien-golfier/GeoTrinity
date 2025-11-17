#include "GeoMovementComponent.h"

#include "GeoCharacter.h"

UGeoMovementComponent::UGeoMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;   // movement driven via ProcessInput calls
}

AGeoCharacter* UGeoMovementComponent::GetGeoCharacter() const
{
	return Cast<AGeoCharacter>(GetOwner());
}
