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
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	virtual void InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData) override;

	virtual void TickPattern(float ServerTime, float SpentTime) override;
	virtual void EndPattern(bool bForceStop = false) override;

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
