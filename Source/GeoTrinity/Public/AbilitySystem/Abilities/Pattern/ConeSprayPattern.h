// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

#include "ConeSprayPattern.generated.h"

class AGeoProjectile;

/**
 * Sprays ProjectileCount projectiles at random angles inside ConeAngle, spread evenly over SprayDuration.
 * Each projectile's angle comes from the payload seed and its own index, so every machine spawns the identical spray,
 * and each is stamped with its own scheduled spawn time so a late tick still places it where it should already be.
 * The cone only covers what the boss faces, which is what makes turning the boss away from the group worth doing.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UConeSprayPattern : public UTickablePattern
{
	GENERATED_BODY()

protected:
	/** Pre-warms the projectile pool for a full spray. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Resets the spawn counter before the new spray starts. */
	virtual void InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData) override;
	/** Spawns every projectile whose scheduled time has passed, and ends once the last one is out. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;

	/** Spawns the projectile at Index with its seed-derived angle and its own scheduled spawn time. */
	void SpawnSprayProjectile(int32 Index) const;
	/** Seconds between two consecutive projectiles of the spray. */
	float GetSpawnInterval() const { return SprayDuration / ProjectileCount; }

	/** Full opening of the spray cone in degrees, centered on the payload yaw. */
	UPROPERTY(EditDefaultsOnly, Category = "Spray", meta = (ClampMin = "0.0"))
	float ConeAngle = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Spray", meta = (ClampMin = "1"))
	int32 ProjectileCount = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Spray", meta = (ClampMin = "0.01"))
	float SprayDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Spray")
	TSubclassOf<AGeoProjectile> ProjectileClass;

private:
	int32 SpawnedCount = 0;
};
