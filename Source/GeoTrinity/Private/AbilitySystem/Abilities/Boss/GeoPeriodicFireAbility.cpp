// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoPeriodicFireAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoPeriodicFireAbility::UGeoPeriodicFireAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGeoPeriodicFireAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	SpawnProjectilesUsingTarget(AbilityTargetData.Yaw, FVector(AbilityTargetData.Origin, ArbitraryCharacterZ),
								AbilityTargetData.ServerSpawnTime);

	GetWorld()->GetTimerManager().SetTimer(FireTriggerTimerHandle, this, &UGeoGameplayAbility::BuildDataAndFire,
										   FMath::FRandRange(FireIntervalMin, FireIntervalMax));
}
