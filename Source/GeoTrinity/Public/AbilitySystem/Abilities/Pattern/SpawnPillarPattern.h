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

protected:
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	virtual FGameplayCueParameters FillCueParam(FAbilityPayload const& Payload) override;

private:
	virtual void InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData) override;
	virtual void ExecuteGameplayCue(FGameplayTag GameplayCueTag) override;
	void SpawnPillarAtLocation(FVector2D const& ZoneLocation, UGeoAbilitySystemComponent* InstigatorAsc) const;
	virtual void StartPattern() override;

	UPROPERTY(EditDefaultsOnly, Category = "FatalZone", meta = (AllowPrivateAccess = "true"))
	float SpawningZoneSize = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "FatalZone", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGeoPillar> PillarClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FatalZone", meta = (AllowPrivateAccess = "true"))
	FDeployableDataParams PillarParams;

	// Effects applied to hostiles in the zone on expiry (server-only).
	UPROPERTY(EditDefaultsOnly, Category = "FatalZone", meta = (AllowPrivateAccess = "true"))
	TArray<TInstancedStruct<FEffectData>> PillarSpawnEffects;

	TSet<FVector2D> PillarSpawnLocations;
};
