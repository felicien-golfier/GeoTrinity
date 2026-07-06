// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoSpawnPillarAbility.generated.h"

/**
 * Boss ability that launches USpawnPillarPattern. Resolves the zone locations once on the server in CreatePatternData
 * (one per targeted player, count scaled by the boss's remaining health) and ships them through PatternStartMulticast,
 * so every client spawns its zones at the exact same positions instead of recomputing from locally-replicated state.
 */
UCLASS()
class GEOTRINITY_API UGeoSpawnPillarAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	virtual TInstancedStruct<FPatternData> CreatePatternData() const override;
};
