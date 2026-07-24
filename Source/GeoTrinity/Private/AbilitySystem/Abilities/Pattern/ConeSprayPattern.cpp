// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/ConeSprayPattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"
#include "Tool/UGeoGameplayLibrary.h"

void UConeSprayPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	if (ensureMsgf(ProjectileParams.ProjectileClass, TEXT("UConeSprayPattern: ProjectileClass is not set")))
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
		SpawnSprayProjectile();
		++SpawnedSalveCount;
	}

	if (SpawnedSalveCount >= SalveNumber)
	{
		EndPattern();
	}
}

void UConeSprayPattern::SpawnSprayProjectile() const
{
	// Scheduled server time of this salve, not the current tick time: on time this spawns the projectiles at the
	// origin, and a tick that fires the salve late stamps them in the past so they fast-forward into place.
	float const SalveSpawnTime = StoredPayload.ServerSpawnTime + StartDelay + SpawnedSalveCount * SalveFrequencySec;

	for (int Index = 0; Index < ProjectileCountPerSalve; ++Index)
	{
		FAbilityPayload ProjectilePayload = StoredPayload;
		ProjectilePayload.Yaw = StoredPayload.Yaw
			+ FMath::Lerp(-ConeAngle * 0.5f, ConeAngle * 0.5f,
						  static_cast<float>(Index) / static_cast<float>(ProjectileCountPerSalve - 1));
		ProjectilePayload.ServerSpawnTime = SalveSpawnTime;

		FTransform const SpawnTransform(FRotator(0.f, ProjectilePayload.Yaw, 0.f),
										FVector(StoredPayload.Origin, ArbitraryCharacterZ));
		GeoASLib::FullySpawnProjectile(GetWorld(), ProjectileParams, SpawnTransform, ProjectilePayload, EffectDataArray,
									   ProjectilePayload.ServerSpawnTime);
	}
}
