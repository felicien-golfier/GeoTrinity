// Fill out your copyright notice in the Description page of Project Settings.

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
	virtual bool ExecuteShot_Implementation() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Projectile")
	TSubclassOf<AGeoProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Target")
	EProjectileTarget Target;
};
