// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Common/GeoDeployAbility.h"
#include "CoreMinimal.h"

#include "GeoMineAbility.generated.h"

/**
 * Instant ability that costs 50% of current HP to deploy a mine.
 * Health guard is enforced in CheckCost so the ability cannot activate when HP is at minimum.
 * LifeSpent is derived from the owner's post-cost HP in AMineSpawnerProjectile::InitDeployable.
 */
UCLASS()
class GEOTRINITY_API UGeoMineAbility : public UGeoDeployAbility
{
	GENERATED_BODY()

protected:
	virtual bool CheckCost(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
						   FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;
};
