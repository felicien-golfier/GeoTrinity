// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "Actor/Projectile/ExternalProjectileParams.h"
#include "CoreMinimal.h"

#include "PeriodicFirePattern.generated.h"

/**
 * Pattern data for UPeriodicFirePattern: one yaw per alive player plus the health-scaled salve count, both resolved on
 * the server by UGeoPeriodicFireAbility. A client only ever sees its own player controller, so it cannot aim at the
 * others itself — the whole burst has to travel with the pattern to play out identically everywhere.
 */
USTRUCT()
struct FPeriodicFirePatternData : public FPatternData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<float> TargetYaws;

	UPROPERTY()
	uint8 SalveCount = 1;
};

/**
 * Fires SalveCount salves SalveInterval apart, each shot sending one projectile along every yaw the ability locked in
 * at fire time. A salve repeats that list once more for each player short of a full party, its repeats spread evenly
 * over the salve, so the burst always carries the same number of projectiles whatever the party size.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UPeriodicFirePattern : public UTickablePattern
{
	GENERATED_BODY()

protected:
	/** Flags a missing projectile class. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Resets the shot counter before the new burst starts. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Spawns the shot whose scheduled time has passed, and ends once the last one is out. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;

	/** Spawns one projectile per target yaw, all stamped with the shot's scheduled spawn time. */
	void SpawnShot(TArray<float> const& TargetYaws, float ShotSpawnTime) const;

	/** Time between two salves. The shots of a single salve split it evenly between them. */
	UPROPERTY(EditDefaultsOnly, Category = "PeriodicFire", meta = (ClampMin = "0.01"))
	float SalveInterval = .3f;

	UPROPERTY(EditDefaultsOnly, Category = "PeriodicFire")
	FExternalProjectileParams ProjectileParams;

private:
	int32 SpawnedShotCount = 0;
};
