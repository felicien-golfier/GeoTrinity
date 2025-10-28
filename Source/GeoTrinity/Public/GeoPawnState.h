// Copyright (c) GeoTrinity
#pragma once

#include "CoreMinimal.h"
#include "InputStep.h"

#include "GeoPawnState.generated.h"

class AGeoPawn;
// Lightweight snapshot of a character's state for saving/serialization/logging.
USTRUCT()
struct FGeoPawnState
{
	GENERATED_BODY()

	// Reference to the pawn this state belongs to (replicated as an object pointer)
	UPROPERTY()
	AGeoPawn* GeoPawn = nullptr;

	// World-space location of the GeoPawn (cm)
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	// World-space orientation of the character
	UPROPERTY()
	FRotator Orientation = FRotator::ZeroRotator;

	// Current velocity (cm/s)
	UPROPERTY()
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

USTRUCT()
struct FGeoGameSnapShot
{
	GENERATED_BODY()

	UPROPERTY()
	FGeoTime ServerTime;

	UPROPERTY()
	TArray<FGeoPawnState> GeoPawnStates;
};