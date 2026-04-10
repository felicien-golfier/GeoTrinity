// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoDeployAbility.generated.h"

/**
 * Hold to charge, release to deploy a projectile at a distance proportional to charge duration.
 * Intended for deployable spawner projectiles (turrets, etc.) shared across player classes.
 * Deploy distance is encoded in the target data Seed field (as integer cm) for server replication.
 */
UCLASS()
class GEOTRINITY_API UGeoDeployAbility : public UGeoProjectileAbility
{
	GENERATED_BODY()

	UGeoDeployAbility();

protected:
	/** Checks the base GAS cost/cooldown and also verifies that the player's deployable manager has room for another. */
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									FGameplayTagContainer const* SourceTags = nullptr,
									FGameplayTagContainer const* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Builds target data encoding the deploy distance (derived from charge ratio) in the Seed field as integer cm. */
	virtual FGeoAbilityTargetData BuildAbilityTargetData() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy")
	float MinDeployDistance = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy")
	float MaxDeployDistance = 1500.f;

	// LifeDrainMaxDuration is used to define the life drain rate base on "How long the deployable would stay alive in
	// sec if nothing else deplete its life", Size is the DeployableSize, for example it is used by the HealingZone to
	// determine the size of the deployable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy")
	FDeployableDataParams Params;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoDeployableBase> DeployableActorClass;

private:
	virtual void SpawnProjectile(FTransform const& SpawnTransform, float SpawnServerTime) const override;
};
