// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"
#include "Actor/Projectile/GeoProjectileParams.h"
#include "CoreMinimal.h"

#include "GeoAutomaticProjectileAbility.generated.h"

enum class EProjectileTarget : uint8;

/**
 * High fire rate ability that spawns projectiles while input is held.
 * Combines the automatic firing loop from UGeoAutomaticFireAbility with projectile spawning.
 */
UCLASS()
class GEOTRINITY_API UGeoAutomaticProjectileAbility : public UGeoAutomaticFireAbility
{
	GENERATED_BODY()

protected:
	/** Spawns a projectile of ProjectileClass aimed according to the Target mode. Returns true on success. */
	virtual bool ExecuteShot_Implementation() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FGeoProjectileParams ProjectileParams;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EProjectileTarget Target;
};
