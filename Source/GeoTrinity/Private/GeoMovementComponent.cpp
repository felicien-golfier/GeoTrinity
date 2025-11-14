#include "GeoMovementComponent.h"

#include "GeoPawn.h"

UGeoMovementComponent::UGeoMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;   // movement driven via ProcessInput calls
}
void UGeoMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{

	if (ShouldSkipUpdate(DeltaTime))
	{
		return;
	}
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent)
	{
		return;
	}

	// Move only on the server; on clients just consume and keep velocity in sync
	if (GetOwnerRole() == ROLE_Authority)
	{
		// Equivalent of base’s local-controller branch
		ApplyControlInputToVelocity(DeltaTime);
		LimitWorldBounds();

		const FVector Delta = Velocity * DeltaTime;
		if (!Delta.IsNearlyZero(1e-6f))
		{
			const FVector OldLocation = UpdatedComponent->GetComponentLocation();
			const FQuat Rotation = UpdatedComponent->GetComponentQuat();
			FHitResult Hit(1.f);
			SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);
			if (Hit.IsValidBlockingHit())
			{
				HandleImpact(Hit, DeltaTime, Delta);
				SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
			}

			const FVector NewLocation = UpdatedComponent->GetComponentLocation();
			Velocity = (NewLocation - OldLocation) / DeltaTime;
		}
		UpdateComponentVelocity();
	}
	else
	{
		// Non-owner clients: do NOT simulate; just clear pending input to avoid buildup
		ConsumeInputVector();
		UpdateComponentVelocity();
	}
}
void UGeoMovementComponent::ApplyControlInputToVelocity(float DeltaTime)
{
	const FVector ControlAcceleration = GetPendingInputVector().GetClampedToMaxSize(1.f);

	const float AnalogInputModifier = (ControlAcceleration.SizeSquared() > 0.f ? ControlAcceleration.Size() : 0.f);
	const float MaxPawnSpeed = GetMaxSpeed() * AnalogInputModifier;
	const bool bExceedingMaxSpeed = IsExceedingMaxSpeed(MaxPawnSpeed);

	if (AnalogInputModifier > 0.f && !bExceedingMaxSpeed)
	{
		// Apply change in velocity direction
		if (Velocity.SizeSquared() > 0.f)
		{
			// Change direction faster than only using acceleration, but never increase velocity magnitude.
			const float TimeScale = FMath::Clamp(DeltaTime * TurningBoost, 0.f, 1.f);
			Velocity = Velocity + (ControlAcceleration * Velocity.Size() - Velocity) * TimeScale;
		}
	}
	else
	{
		// Dampen velocity magnitude based on deceleration.
		if (Velocity.SizeSquared() > 0.f)
		{
			const FVector OldVelocity = Velocity;
			const float VelSize = FMath::Max(Velocity.Size() - FMath::Abs(Deceleration) * DeltaTime, 0.f);
			Velocity = Velocity.GetSafeNormal() * VelSize;

			// Don't allow braking to lower us below max speed if we started above it.
			if (bExceedingMaxSpeed && Velocity.SizeSquared() < FMath::Square(MaxPawnSpeed))
			{
				Velocity = OldVelocity.GetSafeNormal() * MaxPawnSpeed;
			}
		}
	}

	// Apply acceleration and clamp velocity magnitude.
	const float NewMaxSpeed = (IsExceedingMaxSpeed(MaxPawnSpeed)) ? Velocity.Size() : MaxPawnSpeed;
	Velocity += ControlAcceleration * FMath::Abs(Acceleration) * DeltaTime;
	Velocity = Velocity.GetClampedToMaxSize(NewMaxSpeed);

	ConsumeInputVector();
}

bool UGeoMovementComponent::LimitWorldBounds()
{
	AWorldSettings* WorldSettings = PawnOwner ? PawnOwner->GetWorldSettings() : NULL;
	if (!WorldSettings || !WorldSettings->AreWorldBoundsChecksEnabled() || !UpdatedComponent)
	{
		return false;
	}

	const FVector CurrentLocation = UpdatedComponent->GetComponentLocation();
	if (CurrentLocation.Z < WorldSettings->KillZ)
	{
		Velocity.Z = FMath::Min<FVector::FReal>(GetMaxSpeed(), WorldSettings->KillZ - CurrentLocation.Z + 2.0f);
		return true;
	}

	return false;
}

AGeoPawn* UGeoMovementComponent::GetGeoPawn() const
{
	return Cast<AGeoPawn>(GetOwner());
}
