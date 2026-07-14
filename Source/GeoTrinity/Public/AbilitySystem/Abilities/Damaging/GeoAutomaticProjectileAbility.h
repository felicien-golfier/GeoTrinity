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
	/** Spawns a projectile of ProjectileClass aimed according to the Target mode. Returns true on success. */
	virtual bool ExecuteShot_Implementation() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	bool bOverrideDistanceSpan = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability",
			  meta = (ClampMin = "0", AllowPrivateAccess = true, EditCondition = "bOverrideDistanceSpan",
					  EditConditionHides = "true", UIMin = "0"))
	float DistanceSpan = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	bool bOverrideSpeed = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability",
			  meta = (ClampMin = "0", AllowPrivateAccess = true, EditCondition = "bOverrideSpeed",
					  EditConditionHides = "true", UIMin = "0"))
	float ProjectileSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EProjectileTarget Target;
};
