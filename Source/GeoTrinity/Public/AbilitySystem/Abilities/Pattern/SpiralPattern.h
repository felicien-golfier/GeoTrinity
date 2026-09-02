// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
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
	/** Pre-warms the projectile pool for the maximum number of concurrent spiral projectiles. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Computes spiral geometry (angle step, time between projectiles) from the configured round/time parameters. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;

	/** Positions each live projectile along the spiral arc for this tick; ends the pattern when none remain. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** Returns all live projectiles to the pool before delegating to Super. */
	virtual void EndPattern(bool bForceStop = false) override;

	/** Removes Projectile from the active tracking array when it ends its life. */
	UFUNCTION()
	void EndProjectile(AGeoProjectile* Projectile);

	UPROPERTY(EditDefaultsOnly, Category = "GeoSpiral")
	float NumberProjectileByRound;
	UPROPERTY(EditDefaultsOnly, Category = "GeoSpiral")
	float TimeForOneRound;
	UPROPERTY(EditDefaultsOnly, Category = "GeoSpiral")
	float RoundNumber;
	UPROPERTY(EditDefaultsOnly, Category = "GeoSpiral")
	FExternalProjectileParams ProjectileParams;

	UPROPERTY(Transient)
	TArray<AGeoProjectile*> Projectiles;

	float ProjectileSpeed;
	float TimeDiffBetweenProjectiles;
	float AngleBetweenProjectiles;
	FVector FirstProjectileOrientation;
};
