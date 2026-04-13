// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoDetonateAllMinesAbility.generated.h"

/**
 * Instantly detonates all deployed mines with a configurable damage/shield multiplier.
 */
UCLASS()
class GEOTRINITY_API UGeoDetonateAllMinesAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& TargetData,
										  FGameplayTag ApplicationTag) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Detonate", meta = (AllowPrivateAccess = true))
	float DetonationMultiplier = 2.f;
};
