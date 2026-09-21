// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/SpiralPattern.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"

void USpiralPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	ensureMsgf(GetProjectileCount() > 0, TEXT("No projectile set in the spiral ! please fill your pattern values in BP"));
	if (ProjectileParams.ProjectileClass)
	{
		// A first estimation: the first bullets are gone before the last ones are fired.
		UGeoActorPoolingSubsystem::Get(GetWorld())
			->PreSpawn(ProjectileParams.ProjectileClass, static_cast<uint16>(GetProjectileCount() * 0.67f));
	}
}

void USpiralPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	FiredProjectileCount = 0;
	Super::InitPattern(Payload, PatternData);
}

void USpiralPattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	float const TimeBetweenProjectiles = TimeForOneRound / NumberProjectileByRound;
	float const AngleBetweenProjectiles = 360.f / NumberProjectileByRound;
	float const FirstYaw = static_cast<float>(StoredPayload.Seed) / MAX_int32 * 360.f;

	for (; FiredProjectileCount < GetProjectileCount() && FiredProjectileCount * TimeBetweenProjectiles <= SpentTime;
		 ++FiredProjectileCount)
	{
		SpawnSalve({FirstYaw + FiredProjectileCount * AngleBetweenProjectiles},
				   StoredPayload.ServerSpawnTime + StartDelay + FiredProjectileCount * TimeBetweenProjectiles);
	}

	if (FiredProjectileCount >= GetProjectileCount())
	{
		EndPattern();
	}
}

int32 USpiralPattern::GetProjectileCount() const
{
	return FMath::FloorToInt(RoundNumber * NumberProjectileByRound);
}
