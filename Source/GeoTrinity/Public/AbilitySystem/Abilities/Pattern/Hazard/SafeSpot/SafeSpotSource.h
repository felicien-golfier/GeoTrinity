// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"
#include "CoreMinimal.h"

#include "SafeSpotSource.generated.h"

struct FAbilityPayload;

/** Where a pattern takes its safe spots from. */
UENUM()
enum class ESafeSpotSource : uint8
{
	/** Living pillars hostile or neutral to the caster; the radius is the pillar's own. */
	Pillars,
	/** Living adds of the caster's arena (server only; none without an arena); the given radius. */
	Adds,
	/** The given fixed locations; the given radius. */
	Fixed,
};

/** The spots of every source in Sources, all casting Shadow, read as they stand now. Pillars further than MaxDistance
 * from the payload origin are left out (0 = no limit). Radius is the radius of Adds and Fixed spots, FixedLocations
 * the world locations of Fixed spots. */
GEOTRINITY_API TArray<FSafeSpot> GatherSafeSpots(UObject const* WorldContext, FAbilityPayload const& Payload,
												 TArray<ESafeSpotSource> const& Sources, ESafeSpotShadow Shadow,
												 float MaxDistance, float Radius,
												 TArray<FVector2D> const& FixedLocations);
