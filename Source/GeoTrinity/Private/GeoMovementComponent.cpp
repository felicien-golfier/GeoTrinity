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

	// Keep internal 2D box in sync
	GeoPawn->GetBox().Position = FVector2D(NewLocation);
	GeoPawn->SetActorLocation(NewLocation);
}

void UGeoMovementComponent::ApplyCollision(const FGeoBox& Obstacle) const
{
	const AGeoPawn* GeoPawn = GetGeoPawn();
	if (!IsValid(GeoPawn))
	{
		return;
	}
	FGeoBox Box = GeoPawn->GetBox();
	if (!Box.Overlaps(Obstacle))
	{
		return;
	}

	// Correction X
	if (Box.Position.X < Obstacle.Position.X)
	{
		Box.Position.X = Obstacle.Position.X - Obstacle.Size.X - Box.Size.X;
	}
	else
	{
		Box.Position.X = Obstacle.Position.X + Obstacle.Size.X + Box.Size.X;
	}

	// Correction Y
	if (Box.Position.Y < Obstacle.Position.Y)
	{
		Box.Position.Y = Obstacle.Position.Y - Obstacle.Size.Y - Box.Size.Y;
	}
	else
	{
		Box.Position.Y = Obstacle.Position.Y + Obstacle.Size.Y + Box.Size.Y;
	}
}
