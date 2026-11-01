// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/Zone/ZonePattern.h"
#include "AbilitySystem/Data/EffectData.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "SpawnPillarPattern.generated.h"

class AGeoPillar;

/**
 * Zone pattern whose hit brings a pillar on each target location: the pillars drop in during the telegraph, inert, and
 * are released once the hit is judged, so it never touches them. Runs identically on all clients via
 * PatternStartMulticast; only the server spawns.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API USpawnPillarPattern : public UZonePattern
{
	GENERATED_BODY()

public:
	/** Server. Natural end, once every hostile is judged: the pillars finish their arrival, so the hazard never hits
	 * them. Force-stop: they are expired. */
	virtual void EndPattern(bool bForceStop = false) override;

protected:
	/** Flags a missing pillar class so a misconfigured pattern is caught at creation time. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;

	UPROPERTY(EditDefaultsOnly, Category = "GeoPillar")
	TSubclassOf<AGeoPillar> PillarClass;

private:
	/** Server: spawns a pillar per target location, arriving over the wind-up left. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Returns PillarSpawnEffects. */
	virtual TArray<TInstancedStruct<FEffectData>> const& GetHazardEffects() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoPillar", meta = (AllowPrivateAccess = "true"))
	FDeployableDataParams PillarParams;

	// Effects applied to hostiles in the zone on expiry (server-only).
	UPROPERTY(EditDefaultsOnly, Category = "GeoPillar", meta = (AllowPrivateAccess = "true"))
	TArray<TInstancedStruct<FEffectData>> PillarSpawnEffects;

	/** Server. Pillars InitPattern spawned this run. */
	TArray<TWeakObjectPtr<AGeoDeployableBase>> SpawnedPillars;
};
