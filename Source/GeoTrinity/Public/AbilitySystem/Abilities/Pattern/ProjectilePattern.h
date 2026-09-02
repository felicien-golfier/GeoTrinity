// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "CoreMinimal.h"

#include "ProjectilePattern.generated.h"

/**
 * Pattern data for USalvePattern: the yaw of every projectile of the salve, resolved on the server by the launching
 * ability. A client only ever sees its own player controller, so it cannot aim at the others itself — the whole salve
 * has to travel with the pattern to play out identically everywhere.
 */
USTRUCT()
struct FProjectilePatternData : public FPatternData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<float> Yaws;
};

/**
 * Base for every pattern that spawns projectiles: holds the projectile params and the salve spawn loop, so a subclass
 * only decides which yaws leave and when. Spawns nothing on its own — subclasses drive SpawnSalve from TickPattern or
 * StartPattern.
 */
UCLASS(Abstract)
class GEOTRINITY_API UProjectilePattern : public UTickablePattern
{
	GENERATED_BODY()

protected:
	/** Flags a missing projectile class. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;

	/**
	 * Spawns one projectile per yaw at the payload origin, all stamped with the salve's scheduled spawn time.
	 *
	 * @param Yaws            Direction of each projectile of the salve, in degrees.
	 * @param SalveSpawnTime  Server time the salve was scheduled for, not the current tick time: on time this spawns
	 *                        the projectiles at the origin, and a late tick stamps them in the past so they
	 *                        fast-forward into place.
	 */
	void SpawnSalve(TArray<float> const& Yaws, float SalveSpawnTime) const;

	UPROPERTY(EditDefaultsOnly, Category = "GeoProjectile")
	FExternalProjectileParams ProjectileParams;
};

/**
 * Fires the single salve its pattern data carries, then ends. Where it leaves from, where it aims and when it leaves
 * are all the launching ability's call — one launch is one salve.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API USalvePattern : public UProjectilePattern
{
	GENERATED_BODY()

protected:
	/** Flags pattern data that carries no yaws to fire. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Spawns the salve and ends the pattern. */
	virtual void StartPattern() override;
};
