// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Data/EffectData.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "SpawnPillarPattern.generated.h"

class AGeoPillar;

/**
 * Pattern data for USpawnPillarPattern: the zone locations resolved once on the server in USpawnPillarAbility, so every
 * client spawns its zones at the exact same positions instead of recomputing from locally-replicated player state.
 */
USTRUCT()
struct FSpawnPillarPatternData : public FPatternData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector2D> ZoneLocations;
};

/**
 * Pattern that marks a zone under a random player, shows a countdown visual, then on expiry:
 * fires an expiry cue, applies damage to hostiles in the zone, and spawns a GeoPillar.
 * Runs identically on all clients via PatternStartMulticast — server time ensures sync.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API USpawnPillarPattern : public UPattern
{
	GENERATED_BODY()

public:
	/** Turns the hazard on. It keeps the default zero duration: the zones expire at the single instant the pattern goes
	 * live. */
	USpawnPillarPattern();
	/** Server: spawns a pillar at every zone location, unless force-stopped. Coming once every hostile has been
	 * judged, no pillar is simulated around a player who had already left. */
	virtual void EndPattern(bool bForceStop = false) override;

protected:
	/** Flags a missing pillar class so a misconfigured pattern is caught at creation time. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Sets the cue source location to the first zone's world position. */
	virtual FGameplayCueParameters FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload) override;

private:
	/** Reads ZoneLocations from FSpawnPillarPatternData and schedules the per-zone countdown cues. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Fires the zone-indicator cue at every zone location rather than just the pattern origin. */
	virtual void ExecuteGameplayCue(FGeoCueParam const& Cue) override;
	/** True when Location stands in any zone. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** Returns PillarSpawnEffects. */
	virtual TArray<TInstancedStruct<FEffectData>> const& GetHazardEffects() const override;

	UPROPERTY(EditDefaultsOnly, Category = "GeoPillar", meta = (AllowPrivateAccess = "true"))
	float SpawningZoneSize = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "GeoPillar", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGeoPillar> PillarClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoPillar", meta = (AllowPrivateAccess = "true"))
	FDeployableDataParams PillarParams;

	// Effects applied to hostiles in the zone on expiry (server-only).
	UPROPERTY(EditDefaultsOnly, Category = "GeoPillar", meta = (AllowPrivateAccess = "true"))
	TArray<TInstancedStruct<FEffectData>> PillarSpawnEffects;

	UPROPERTY(EditDefaultsOnly, Category = "GeoPillar", meta = (AllowPrivateAccess = "true"))
	FGeoCueParam DirectionCue;

	TSet<FVector2D> PillarSpawnLocations;
};
