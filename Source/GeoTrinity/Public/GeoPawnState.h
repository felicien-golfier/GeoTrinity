// Copyright (c) GeoTrinity
#pragma once

#include "CoreMinimal.h"
#include "InputStep.h"

class AGeoPawn;
// Lightweight snapshot of a character's state for saving/serialization/logging.
struct FGeoPawnState
{
	TWeakObjectPtr<AGeoPawn> GeoPawn;

	// World-space location of the GeoPawn (cm)
	FVector Location = FVector::ZeroVector;

	// World-space orientation of the character
	FRotator Orientation = FRotator::ZeroRotator;

	// Current velocity (cm/s)
	FVector Velocity = FVector::ZeroVector;

	FGeoPawnState() = default;

	FGeoPawnState(AGeoPawn* InGeoPawn, const FVector& InLocation, const FRotator& InOrientation, const FVector& InVelocity)
		: GeoPawn(InGeoPawn)
		, Location(InLocation)
		, Orientation(InOrientation)
		, Velocity(InVelocity)
	{
	}
};

struct FGeoGameSnapShot
{
	FGeoTime ServerTime;
	TArray<FGeoPawnState> GeoPawnStates;
};