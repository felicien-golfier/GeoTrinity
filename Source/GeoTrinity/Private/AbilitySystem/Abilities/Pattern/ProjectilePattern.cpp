// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/ProjectilePattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "Tool/UGeoGameplayLibrary.h"

void UProjectilePattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);

	ensureMsgf(ProjectileParams.ProjectileClass, TEXT("%s: ProjectileClass is not set"), *GetName());
}

void UProjectilePattern::SpawnSalve(TArray<float> const& Yaws, float const SalveSpawnTime) const
{
	FVector const SpawnLocation(StoredPayload.Origin, ArbitraryCharacterZ);

	for (float const Yaw : Yaws)
	{
		FAbilityPayload ProjectilePayload = StoredPayload;
		ProjectilePayload.Yaw = Yaw;
		ProjectilePayload.ServerSpawnTime = SalveSpawnTime;

		GeoASLib::FullySpawnProjectile(GetWorld(), ProjectileParams, FTransform(FRotator(0.f, Yaw, 0.f), SpawnLocation),
									   ProjectilePayload, EffectDataArray, SalveSpawnTime);
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
