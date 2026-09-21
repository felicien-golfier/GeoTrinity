// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/ProjectilePattern.h"
#include "CoreMinimal.h"

#include "SpiralPattern.generated.h"

/**
 * Bullet pattern that fires one bullet at a time, each turned a step further than the last, so they draw expanding
 * spirals. The first bullet's direction comes from the payload seed, so every machine fires the identical spiral, and
 * each bullet is stamped with its own scheduled spawn time so a late tick still places it where it should already be.
 */
UCLASS()
class GEOTRINITY_API USpiralPattern : public UProjectilePattern
{
	GENERATED_BODY()

protected:
	/** Flags a spiral with no bullet and pre-warms the projectile pool. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Resets the fired counter before the new spiral starts. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Fires every bullet whose scheduled time has passed, and ends once the last one is out. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;

	/** How many bullets the whole spiral fires. */
	int32 GetProjectileCount() const;

	UPROPERTY(EditDefaultsOnly, Category = "GeoSpiral")
	float NumberProjectileByRound;
	UPROPERTY(EditDefaultsOnly, Category = "GeoSpiral")
	float TimeForOneRound;
	UPROPERTY(EditDefaultsOnly, Category = "GeoSpiral")
	float RoundNumber;

private:
	int32 FiredProjectileCount = 0;
};
