// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/ConeSprayPattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "Tool/UGeoGameplayLibrary.h"

void UConeSprayPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (ensureMsgf(ProjectileClass, TEXT("UConeSprayPattern: ProjectileClass is not set")))
	{
		UGeoActorPoolingSubsystem::Get(GetWorld())
			->PreSpawn(ProjectileClass, static_cast<uint16>(ProjectileCountPerSalve * SalveNumber));
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
		SpawnSprayProjectile(SpentTime);
		++SpawnedSalveCount;
	}

	if (SpawnedSalveCount >= SalveNumber)
	{
		EndPattern();
	}
}

void UConeSprayPattern::SpawnSprayProjectile(float const SpentTime) const
{
	for (int Index = 0; Index < ProjectileCountPerSalve; ++Index)
	{
		FAbilityPayload ProjectilePayload = StoredPayload;
		ProjectilePayload.Yaw = StoredPayload.Yaw
			+ FMath::Lerp(-ConeAngle * 0.5f, ConeAngle * 0.5f,
						  static_cast<float>(Index) / static_cast<float>(ProjectileCountPerSalve - 1));
		ProjectilePayload.ServerSpawnTime = StoredPayload.ServerSpawnTime + SpentTime;

		FTransform const SpawnTransform(FRotator(0.f, ProjectilePayload.Yaw, 0.f),
										FVector(StoredPayload.Origin, ArbitraryCharacterZ));
		GeoASLib::FullySpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, ProjectilePayload, EffectDataArray,
									   ProjectilePayload.ServerSpawnTime);
	}
}
