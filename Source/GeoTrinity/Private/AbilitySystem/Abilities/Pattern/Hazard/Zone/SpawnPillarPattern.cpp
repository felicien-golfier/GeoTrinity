// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Pattern/Hazard/Zone/SpawnPillarPattern.h"

#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Deployable/Pillar/GeoPillar.h"
#include "Tool/UGeoGameplayLibrary.h"

void USpawnPillarPattern::OnCreate(FGameplayTag const AbilityTag, AActor& Owner)
{
	Super::OnCreate(AbilityTag, Owner);
	// TODO: why not having deployables in the pooling system. Need to set it up properly
	//  UGeoActorPoolingSubsystem::Get(GetWorld())->PreSpawn(PillarClass, 10);

	ensureMsgf(IsValid(PillarClass), TEXT("%hs: PillarClass is not set on %s"), __FUNCTION__, *GetName());
}

void USpawnPillarPattern::InitPattern(FAbilityPayload const& Payload, TInstancedStruct<FPatternData> const& PatternData)
{
	SpawnedPillars.Empty();
	Super::InitPattern(Payload, PatternData);

	if (IsPatternActive() && GeoLib::IsServer(GetWorld()))
	{
		FDeployableDataParams SpawnParams = PillarParams;
		SpawnParams.InitDuration = FMath::Max(0.f, StartDelay - TravelTime);
		SpawnParams.bAutoFinishInit = false;
		for (FVector2D const& Location : GetTargetLocations())
		{
			SpawnedPillars.Add(GeoASLib::FullySpawnDeployable(PillarClass, StoredPayload, EffectDataArray, SpawnParams,
															  FTransform(FVector(Location, ArbitraryCharacterZ))));
		}
	}
}

TArray<TInstancedStruct<FEffectData>> const& USpawnPillarPattern::GetHazardEffects() const
{
	return PillarSpawnEffects;
}

void USpawnPillarPattern::EndPattern(bool const bForceStop)
{
	if (IsPatternActive() && GeoLib::IsServer(GetWorld()))
	{
		for (TWeakObjectPtr<AGeoDeployableBase> const& Pillar : SpawnedPillars)
		{
			if (Pillar.IsValid() && bForceStop)
			{
				Pillar->Expire(true);
			}
			else if (Pillar.IsValid())
			{
				Pillar->FinishInit();
			}
		}
	}

	Super::EndPattern(bForceStop);
}
