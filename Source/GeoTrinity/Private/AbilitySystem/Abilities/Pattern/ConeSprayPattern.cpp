// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/ConeSprayPattern.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"

void UConeSprayPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (ProjectileParams.ProjectileClass)
	{
		UGeoActorPoolingSubsystem::Get(GetWorld())
			->PreSpawn(ProjectileParams.ProjectileClass, static_cast<uint16>(ProjectileCountPerSalve * SalveNumber));
	}
}

void UConeSprayPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	SpawnedSalveCount = 0;
	Super::InitPattern(Payload, PatternData);
}

void UConeSprayPattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	if (SpentTime > SpawnedSalveCount * SalveFrequencySec)
	{
		SpawnSpraySalve();
		++SpawnedSalveCount;
	}

	if (SpawnedSalveCount >= SalveNumber)
	{
		EndPattern();
	}
}

void UConeSprayPattern::SpawnSpraySalve() const
{
	TArray<float> Yaws;
	Yaws.Reserve(ProjectileCountPerSalve);
	for (int Index = 0; Index < ProjectileCountPerSalve; ++Index)
	{
		Yaws.Add(StoredPayload.Yaw
				 + FMath::Lerp(-ConeAngle * 0.5f, ConeAngle * 0.5f,
							   static_cast<float>(Index) / static_cast<float>(ProjectileCountPerSalve - 1)));
	}

	SpawnSalve(Yaws, StoredPayload.ServerSpawnTime + StartDelay + SpawnedSalveCount * SalveFrequencySec);
}
