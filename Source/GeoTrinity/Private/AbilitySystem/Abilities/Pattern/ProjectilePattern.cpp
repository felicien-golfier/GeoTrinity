// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/ProjectilePattern.h"

#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoBulletSubsystem.h"

void UProjectilePattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	ensureMsgf(ProjectileParams.ProjectileClass, TEXT("%s: ProjectileClass is not set"), *GetName());

	for (TInstancedStruct<FEffectData> const& Effect : EffectDataArray)
	{
		FEffectData const* const EffectData = Effect.GetPtr();
		ensureMsgf(!EffectData || !EffectData->IsPerSecond(),
				   TEXT("%s: a bullet carries a per-second effect, and a bullet only ever hits for an instant"),
				   *GetName());
	}
}

void UProjectilePattern::SpawnSalve(TArray<float> const& Yaws, float const SalveSpawnTime) const
{
	UGeoBulletSubsystem* const BulletSubsystem = UGeoBulletSubsystem::Get(GetWorld());

	for (float const Yaw : Yaws)
	{
		FAbilityPayload BulletPayload = StoredPayload;
		BulletPayload.Yaw = Yaw;
		BulletPayload.ServerSpawnTime = SalveSpawnTime;

		BulletSubsystem->FireBullet(BulletPayload, ProjectileParams, EffectDataArray, TeamAttitude,
									/*bSeenThroughReplication*/ false);
	}
}

void USalvePattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	ensureMsgf(PatternData.GetPtr<FProjectilePatternData>(),
			   TEXT("USalvePattern: PatternData is not an FProjectilePatternData — launch this pattern from an ability "
					"filling the salve yaws"));

	Super::InitPattern(Payload, PatternData);
}

void USalvePattern::StartPattern()
{
	Super::StartPattern();

	if (FProjectilePatternData const* SalveData = StoredPatternData.GetPtr<FProjectilePatternData>())
	{
		SpawnSalve(SalveData->Yaws, StoredPayload.ServerSpawnTime + StartDelay);
	}

	EndPattern();
}
