// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoPeriodicFireAbility.h"

#include "AbilitySystem/Abilities/Pattern/PeriodicFirePattern.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoPeriodicFireAbility::UGeoPeriodicFireAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

TInstancedStruct<FPatternData> UGeoPeriodicFireAbility::CreatePatternData() const
{
	FPeriodicFirePatternData FireData;

	UGeoAttributeSetBase const* AttributeSet = Cast<UGeoAttributeSetBase>(
		GetGeoAbilitySystemComponentFromActorInfo()->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
	if (!ensureMsgf(IsValid(AttributeSet), TEXT("GeoPeriodicFireAbility: OwnerASC has no UGeoAttributeSetBase")))
	{
		return TInstancedStruct<FPatternData>::Make<FPeriodicFirePatternData>(FireData);
	}

	float const HealthRatio = AttributeSet->GetHealthRatio();
	if (HealthRatio < .2f)
	{
		FireData.SalveCount = 3;
	}
	else if (HealthRatio < .5f)
	{
		FireData.SalveCount = 2;
	}
	else
	{
		FireData.SalveCount = 1;
	}

	FVector const Origin(StoredPayload.Origin, ArbitraryCharacterZ);
	for (APlayableCharacter const* Player : GeoLib::GetAlivePlayers(this))
	{
		FireData.TargetYaws.Add((Player->GetActorLocation() - Origin).Rotation().Yaw);
	}

	return TInstancedStruct<FPatternData>::Make<FPeriodicFirePatternData>(FireData);
}

void UGeoPeriodicFireAbility::LaunchPattern()
{
	Super::LaunchPattern();

	// Super ends the ability when its pattern instance is missing; scheduling then would relaunch a dead ability.
	if (IsActive())
	{
		FRandomStream Stream(StoredPayload.Seed);
		GetWorld()->GetTimerManager().SetTimer(FireTriggerTimerHandle, this, &UGeoPeriodicFireAbility::LaunchPattern,
											   Stream.FRandRange(FireIntervalMin, FireIntervalMax));
	}
}
