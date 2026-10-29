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

/**
 * Gathers the safe spots from every source in Sources, all casting Shadow, read as they stand now.
 *
 * @param WorldContext     World context object.
 * @param Payload          Firing payload; pillar distance is measured from Payload.Origin.
 * @param Sources          Which sources to gather spots from.
 * @param Shadow           Shadow type each gathered spot casts.
 * @param MaxDistance      Furthest a pillar may be from Payload.Origin to count. 0 = no limit.
 * @param Radius           Radius of Adds and Fixed spots (world units).
 * @param FixedLocations   World-space locations of Fixed spots.
 */
GEOTRINITY_API TArray<FSafeSpot> GatherSafeSpots(UObject const* WorldContext, FAbilityPayload const& Payload,
												 TArray<ESafeSpotSource> const& Sources, ESafeSpotShadow Shadow,
												 float MaxDistance, float Radius,
												 TArray<FVector2D> const& FixedLocations);
