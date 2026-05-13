// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "FatalZonePattern.generated.h"

class AGeoPillar;

/**
 * Pattern that marks a zone under a random player, shows a countdown visual, then on expiry:
 * fires an expiry cue, applies damage to hostiles in the zone, and spawns a GeoPillar.
 * Runs identically on all clients via PatternStartMulticast — server time ensures sync.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UFatalZonePattern : public UPattern
{
	GENERATED_BODY()

protected:
	virtual void OnCreate(FGameplayTag AbilityTag) override;
	virtual void InitPattern(FAbilityPayload const& Payload) override;

private:
	UFUNCTION()
	void OnExpire();

	FTimerHandle ExpiryTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "FatalZone")
	float CountdownDuration = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "FatalZone")
	float ZoneSize = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "FatalZone")
	TSubclassOf<AGeoPillar> PillarClass;

	UPROPERTY(EditDefaultsOnly, Category = "FatalZone")
	FGameplayTag CountdownGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "FatalZone")
	FGameplayTag ExpiryGameplayCueTag;

	// Effects applied to hostiles in the zone on expiry (server-only).
	UPROPERTY(EditDefaultsOnly, Category = "FatalZone")
	TArray<TInstancedStruct<FEffectData>> ZoneEffectDataArray;

	// Effects passed into the spawned pillar's FDeployableData.
	UPROPERTY(EditDefaultsOnly, Category = "FatalZone")
	TArray<TInstancedStruct<FEffectData>> PillarEffectDataArray;
};
