// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoSpawnPillarAbility.generated.h"

/** One chosen pillar: the character its zone follows, and where to drop it should that character be gone by launch. */
struct FPillarTarget
{
	TWeakObjectPtr<AActor> Target;
	// Scatter of a surplus pillar, keeping it beside the character it shares instead of on top of the other pillar.
	FVector2D Offset;
	// Zone position when the pillar was chosen, Offset included. Only read once Target no longer exists.
	FVector2D FallbackLocation;
};

/**
 * Boss ability that launches USpawnPillarPattern. Picks its targets one PreLaunchDelay ahead of the launch, marks each
 * of them with the telegraph cue, then resolves the zone locations on the server in CreatePatternData and ships them
 * through PatternStartMulticast, so every client spawns its zones at the exact same positions instead of recomputing
 * from locally-replicated state. The pillar count depends on the boss's health alone, never on how many players are
 * alive: pillars beyond the alive count land at a random offset around an alive player.
 */
UCLASS()
class GEOTRINITY_API UGeoSpawnPillarAbility : public UPatternAbility
{
	GENERATED_BODY()

protected:
	/**
	 * Server-only. Reads the boss's current health ratio to choose 1–3 pillar count, seeds the target pick and the
	 * scatter offsets from LaunchSeed, and marks every chosen character with the telegraph cue. Runs before
	 * PreLaunchDelay, so the pick is made once and the launch only reads it back.
	 */
	virtual void BeginPreLaunch() override;

	/** Returns an FSpawnPillarPatternData whose ZoneLocations follow the marked characters to wherever they stand at
	 * launch — falling back to where each was marked once it is gone, so a death mid-cast drops no pillar. */
	virtual TInstancedStruct<FPatternData> CreatePatternData() const override;

	/** Distance range a surplus pillar is offset by from the alive player it falls back to. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Pillar")
	float MinScatterRadius = 300.f;

	/** Upper bound of the scatter-offset distance. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Pillar")
	float MaxScatterRadius = 800.f;

	TArray<FPillarTarget> PillarTargets;
};
