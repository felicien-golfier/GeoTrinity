// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoPeriodicFireAbility.generated.h"

/**
 * Launches a UPeriodicFirePattern burst at all players, again and again: the ability never ends on its own, every burst
 * schedules the next one, so a single activation keeps the boss firing.
 * Each burst carries a fresh server time stamp plus the yaws and salve count resolved here, so every machine plays out
 * the same shots instead of watching replicated ones.
 */
UCLASS()
class GEOTRINITY_API UGeoPeriodicFireAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	/** Configures ServerOnly net execution, InstancedPerActor instancing, and no replication. */
	UGeoPeriodicFireAbility();

protected:
	/** Aims one yaw at each alive player and scales the salve count on the boss health, for the burst about to start.
	 */
	virtual TInstancedStruct<FPatternData> CreatePatternData() const override;

	/** Schedules the next burst, one fire interval after this one starts, so the rhythm never stretches with the burst
	 * length. */
	virtual void LaunchPattern() override;

	/** A finished burst ends nothing: the next one was already scheduled when this one started. */
	virtual void OnPatternEnd() override {}

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.1"))
	float FireIntervalMin = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.1"))
	float FireIntervalMax = 5.f;
};
