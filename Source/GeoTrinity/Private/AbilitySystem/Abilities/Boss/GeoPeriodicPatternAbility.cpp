// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Boss/GeoPeriodicPatternAbility.h"

#include "AbilitySystem/Abilities/Pattern/ProjectilePattern.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Tool/UGeoGameplayLibrary.h"

// Party size the burst is balanced around: every player short of it adds one salve to each round.
static constexpr int32 ReferencePlayerCount = 3;

UGeoPeriodicPatternAbility::UGeoPeriodicPatternAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

void UGeoPeriodicPatternAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
												 FGameplayAbilityActorInfo const* ActorInfo,
												 FGameplayAbilityActivationInfo const ActivationInfo,
												 FGameplayEventData const* TriggerEventData)
{
	RemainingSalveCount = 0;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

TInstancedStruct<FPatternData> UGeoPeriodicPatternAbility::CreatePatternData() const
{
	FProjectilePatternData SalveData;

	FVector const Origin(StoredPayload.Origin, ArbitraryCharacterZ);
	for (APlayableCharacter const* Player : GeoLib::GetAlivePlayers(this))
	{
		SalveData.Yaws.Add((Player->GetActorLocation() - Origin).Rotation().Yaw);
	}

	return TInstancedStruct<FPatternData>::Make<FProjectilePatternData>(SalveData);
}

void UGeoPeriodicPatternAbility::SizeBurst()
{
	int32 RoundCount = 1;
	UGeoAttributeSetBase const* AttributeSet = Cast<UGeoAttributeSetBase>(
		GetGeoAbilitySystemComponentFromActorInfo()->GetAttributeSet(UGeoAttributeSetBase::StaticClass()));
	if (ensureMsgf(IsValid(AttributeSet), TEXT("GeoPeriodicFireAbility: OwnerASC has no UGeoAttributeSetBase")))
	{
		float const HealthRatio = AttributeSet->GetHealthRatio();
		if (HealthRatio < .2f)
		{
			RoundCount = 3;
		}
		else if (HealthRatio < .5f)
		{
			RoundCount = 2;
		}
	}

	int32 const SalvesPerRound = FMath::Max(1, ReferencePlayerCount - GeoLib::GetAlivePlayers(this).Num() + 1);
	TimeBetweenSalves = RoundDuration / SalvesPerRound;
	RemainingSalveCount = RoundCount * SalvesPerRound;

	ensureMsgf(GetFireDelay() <= TimeBetweenSalves,
			   TEXT("GeoPeriodicFireAbility: FireDelay (%f) is longer than the time between two salves (%f) — the "
					"pattern instance is shared, so a salve still winding up is dropped by the next one"),
			   GetFireDelay(), TimeBetweenSalves);
}

void UGeoPeriodicPatternAbility::LaunchPattern()
{
	if (RemainingSalveCount <= 0)
	{
		SizeBurst();
	}

	Super::LaunchPattern();

	// Super ends the ability when its pattern instance is missing; scheduling then would relaunch a dead ability.
	if (!IsActive())
	{
		return;
	}

	--RemainingSalveCount;
	float NextSalveDelay = TimeBetweenSalves;
	if (RemainingSalveCount <= 0)
	{
		FRandomStream Stream(StoredPayload.Seed);
		NextSalveDelay = Stream.FRandRange(FireIntervalMin, FireIntervalMax);
	}

	LaunchSeed = GetNewSeed();
	GetWorld()->GetTimerManager().SetTimer(FireTriggerTimerHandle, this, &UGeoPeriodicPatternAbility::LaunchPattern,
										   NextSalveDelay);
}
