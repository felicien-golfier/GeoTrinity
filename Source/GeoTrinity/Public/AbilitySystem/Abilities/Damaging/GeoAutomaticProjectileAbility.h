// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"
#include "CoreMinimal.h"

#include "GeoAutomaticProjectileAbility.generated.h"

enum class EProjectileTarget : uint8;
class AGeoProjectile;

/**
 * High fire rate ability that spawns projectiles while input is held.
 * Combines the automatic firing loop from UGeoAutomaticFireAbility with projectile spawning.
 */
UCLASS()
class GEOTRINITY_API UGeoAutomaticProjectileAbility : public UGeoAutomaticFireAbility
{
	GENERATED_BODY()

protected:
	/** Builds target data from the character's current position and facing, encoding Target mode into the payload. */
	virtual FGeoAbilityTargetData BuildAbilityTargetData() override;
	/** Spawns a projectile of ProjectileClass aimed according to the Target mode. Returns true on success. */
	virtual bool ExecuteShot_Implementation() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EProjectileTarget Target;
};
