// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoTileBombAbility.generated.h"

/**
 * Boss ability that sticks a bomb to one live player and launches UTileBombPattern. The carrier is drawn from the
 * payload seed once on the server and shipped through PatternStartMulticast, so every machine shows the countdown on
 * the same player instead of picking its own from locally-replicated state.
 */
UCLASS()
class GEOTRINITY_API UGeoTileBombAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	/** Draws the bomb carrier from live players using the payload seed so every machine attaches the countdown to the same player. */
	virtual TInstancedStruct<FPatternData> CreatePatternData() const override;
};
