// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoSpawnPillarAbility.generated.h"

/**
 * Boss ability that launches USpawnPillarPattern. Resolves the zone locations once on the server in CreatePatternData
 * and ships them through PatternStartMulticast, so every client spawns its zones at the exact same positions instead
 * of recomputing from locally-replicated state. The pillar count depends on the boss's health alone, never on how many
 * players are alive: pillars beyond the alive count land at a random offset around an alive player.
 */
UCLASS()
class GEOTRINITY_API UGeoSpawnPillarAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	/**
	 * Server-only. Reads the boss's current health ratio to choose 1–3 pillar count, sorts alive players by PlayerId
	 * for determinism, seeds offsets from StoredPayload.Seed, and returns an FSpawnPillarPatternData carrying the
	 * resolved ZoneLocations — so every client spawns pillars at identical positions without recomputing.
	 */
	virtual TInstancedStruct<FPatternData> CreatePatternData() const override;

	/** Distance range a surplus pillar is offset by from the alive player it falls back to. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Pillar")
	float MinScatterRadius = 300.f;

	/** Upper bound of the scatter-offset distance. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Pillar")
	float MaxScatterRadius = 800.f;
};
