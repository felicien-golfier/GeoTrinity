// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoPeriodicFireAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
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

	UGeoAbilitySystemComponent* GeoAsc = GetGeoAbilitySystemComponentFromActorInfo();
	UGeoAttributeSetBase const* AttributeSet =
		Cast<UGeoAttributeSetBase>(GeoAsc->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));

	if (!IsValid(AttributeSet))
	{
		ensureMsgf(AttributeSet, TEXT("SpawnPillarPattern: OwnerASC has no UGeoAttributeSetBase"));
		return;
	}

	float const HealthRatio = AttributeSet->GetHealthRatio();
	bool bShouldSalve = SalveCount < 3;

	if (HealthRatio < .2f)
	{
		bShouldSalve = SalveCount < 2;
	}
	else if (HealthRatio < .5f)
	{
		bShouldSalve = SalveCount < 1;
	}
	else
	{
		bShouldSalve = false;
	}

	float const Time = bShouldSalve ? SalveInterval : FMath::FRandRange(FireIntervalMin, FireIntervalMax);
	GetWorld()->GetTimerManager().SetTimer(FireTriggerTimerHandle, this, &UGeoGameplayAbility::BuildDataAndFire, Time);
	SalveCount = bShouldSalve ? SalveCount + 1 : 0;
}
