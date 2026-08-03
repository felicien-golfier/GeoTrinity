// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/PeriodicFirePattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/UGeoGameplayLibrary.h"

// Party size the burst is balanced around: every player short of it adds one repeat to each salve.
static constexpr int32 ReferencePlayerCount = 3;

void UPeriodicFirePattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	ensureMsgf(ProjectileParams.ProjectileClass, TEXT("UPeriodicFirePattern: ProjectileClass is not set"));
}

void UPeriodicFirePattern::InitPattern(FAbilityPayload const& Payload,
									   TInstancedStruct<FPatternData> const& PatternData)
{
	SpawnedShotCount = 0;
	ensureMsgf(PatternData.GetPtr<FPeriodicFirePatternData>(),
			   TEXT("UPeriodicFirePattern: PatternData is not an FPeriodicFirePatternData — launch this pattern from "
					"UGeoPeriodicFireAbility"));

	Super::InitPattern(Payload, PatternData);
}

void UPeriodicFirePattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	FPeriodicFirePatternData const* FireData = StoredPatternData.GetPtr<FPeriodicFirePatternData>();
	if (!FireData)
	{
		EndPattern();
		return;
	}

	int32 const MissingPlayerCount = ReferencePlayerCount - FireData->TargetYaws.Num();
	int32 const ShotsPerSalve = FMath::Max(1, MissingPlayerCount + 1);
	float const ShotInterval = SalveInterval / ShotsPerSalve;

	if (SpentTime > SpawnedShotCount * ShotInterval)
	{
		// Scheduled server time of this shot, not the current tick time: on time this spawns the projectiles at the
		// origin, and a tick that fires the shot late stamps them in the past so they fast-forward into place.
		SpawnShot(FireData->TargetYaws, StoredPayload.ServerSpawnTime + StartDelay + SpawnedShotCount * ShotInterval);
		++SpawnedShotCount;
	}

	if (SpawnedShotCount >= FireData->SalveCount * ShotsPerSalve)
	{
		EndPattern();
	}
}

void UPeriodicFirePattern::SpawnShot(TArray<float> const& TargetYaws, float const ShotSpawnTime) const
{
	FVector const SpawnLocation(StoredPayload.Origin, ArbitraryCharacterZ);

	for (float const Yaw : TargetYaws)
	{
		FAbilityPayload ProjectilePayload = StoredPayload;
		ProjectilePayload.Yaw = Yaw;
		ProjectilePayload.ServerSpawnTime = ShotSpawnTime;

		GeoASLib::FullySpawnProjectile(GetWorld(), ProjectileParams, FTransform(FRotator(0.f, Yaw, 0.f), SpawnLocation),
									   ProjectilePayload, EffectDataArray, ShotSpawnTime);
	}
}
