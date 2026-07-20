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
		UGeoActorPoolingSubsystem::Get(GetWorld())->PreSpawn(ProjectileClass, static_cast<uint16>(ProjectileCount));
	}
}

void UConeSprayPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	SpawnedCount = 0;
	Super::InitPattern(Payload, PatternData);
}

void UConeSprayPattern::TickPattern(float /*ServerTime*/, float const SpentTime)
{
	int32 const DueCount = FMath::Min(ProjectileCount, FMath::FloorToInt32(SpentTime / GetSpawnInterval()) + 1);
	while (SpawnedCount < DueCount)
	{
		SpawnSprayProjectile(SpawnedCount++);
	}

	if (SpawnedCount >= ProjectileCount)
	{
		EndPattern();
	}
}

void UConeSprayPattern::SpawnSprayProjectile(int32 const Index) const
{
	// Seeding per index rather than drawing from one stream keeps the angle of a given projectile identical on every
	// machine, even when a machine catches up on several projectiles in the same tick.
	FRandomStream Stream(StoredPayload.Seed + Index);

	FAbilityPayload ProjectilePayload = StoredPayload;
	ProjectilePayload.Yaw = StoredPayload.Yaw + Stream.FRandRange(-ConeAngle * 0.5f, ConeAngle * 0.5f);
	ProjectilePayload.ServerSpawnTime = StoredPayload.ServerSpawnTime + StartDelay + Index * GetSpawnInterval();

	FTransform const SpawnTransform(FRotator(0.f, ProjectilePayload.Yaw, 0.f),
									FVector(StoredPayload.Origin, ArbitraryCharacterZ));
	GeoASLib::FullySpawnProjectile(GetWorld(), ProjectileClass, SpawnTransform, ProjectilePayload, EffectDataArray,
								   ProjectilePayload.ServerSpawnTime);
}
