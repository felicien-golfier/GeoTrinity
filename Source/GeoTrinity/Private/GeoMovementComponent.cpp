#include "GeoMovementComponent.h"

#include "GeoPawn.h"

UGeoMovementComponent::UGeoMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;   // movement driven via ProcessInput calls
	SetIsReplicatedByDefault(true);
}

AGeoPawn* UGeoMovementComponent::GetGeoPawn() const
{
	return Cast<AGeoPawn>(GetOwner());
}

void UGeoMovementComponent::MovePawnWithInput(float DeltaTime, FVector2D GivenMovementInput)
{
	AGeoPawn* GeoPawn = GetGeoPawn();
	if (!IsValid(GeoPawn))
	{
		return;
	}

	if (GivenMovementInput.IsZero())
	{
		// if no input, velocity decays to zero
		Velocity = FVector::ZeroVector;
		return;
	}

	// Scale our movement input axis values by 100 units per second
	GivenMovementInput = GivenMovementInput.GetSafeNormal() * 100.0f;

	const FVector OldLocation = GeoPawn->GetActorLocation();
	FVector NewLocation = OldLocation;

	NewLocation += GeoPawn->GetActorForwardVector() * GivenMovementInput.Y * DeltaTime;
	NewLocation += GeoPawn->GetActorRightVector() * GivenMovementInput.X * DeltaTime;

	// Update velocity based on displacement
	const FVector DeltaLocation = NewLocation - OldLocation;
	Velocity = (DeltaTime > SMALL_NUMBER) ? (DeltaLocation / DeltaTime) : FVector::ZeroVector;

	GeoPawn->SetActorLocation(NewLocation);
}

void UGeoMovementComponent::ApplyCollision(const FBox2D& Obstacle) const
{
	const AGeoPawn* GeoPawn = GetGeoPawn();
	if (!IsValid(GeoPawn))
	{
		return;
	}

	FBox2D Box = GeoPawn->GetBox();
	if (!Box.Intersect(Obstacle))
	{
		return;
	}

	// Do Correction on location
}
