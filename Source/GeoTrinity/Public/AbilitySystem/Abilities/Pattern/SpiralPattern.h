// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "Actor/Projectile/GeoProjectileParams.h"
#include "CoreMinimal.h"

#include "SpiralPattern.generated.h"

/**
 * Bullet pattern that fires projectiles in expanding spirals.
 * NumberProjectileByRound, TimeForOneRound, and RoundNumber configure the density and duration.
 */
UCLASS()
class GEOTRINITY_API USpiralPattern : public UTickablePattern
{
	GENERATED_BODY()
protected:
	/** Pre-warms the projectile pool, computes per-projectile timing and angle constants from the designer properties. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Maps the payload seed to a deterministic starting angle so two spiral instances never overlap identically. */
	virtual void InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData) override;

	/** Spawns and positions projectiles whose scheduled time has passed; ends once all rounds are out and no live projectiles remain. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** Releases all active spiral projectiles back to the pool and delegates to the base cleanup. */
	virtual void EndPattern(bool bForceStop = false) override;

	/** Removes Projectile from the tracked list and unbinds this callback when a projectile ends its life early. */
	UFUNCTION()
	void EndProjectile(AGeoProjectile* Projectile);

	UPROPERTY(EditDefaultsOnly, Category = "Spiral")
	float NumberProjectileByRound;
	UPROPERTY(EditDefaultsOnly, Category = "Spiral")
	float TimeForOneRound;
	UPROPERTY(EditDefaultsOnly, Category = "Spiral")
	float RoundNumber;
	UPROPERTY(EditDefaultsOnly, Category = "Spiral")
	FGeoProjectileParams ProjectileParams;

	UPROPERTY(Transient)
	TArray<AGeoProjectile*> Projectiles;

	float ProjectileSpeed;
	float TimeDiffBetweenProjectiles;
	float AngleBetweenProjectiles;
	FVector FirstProjectileOrientation;
};
