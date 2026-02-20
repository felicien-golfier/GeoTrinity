// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Tool/UGameplayLibrary.h"

UGeoAutomaticFireAbility::UGeoAutomaticFireAbility()
{
	bRetriggerInstancedAbility = false;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UGeoAutomaticFireAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											   FGameplayAbilityActorInfo const* ActorInfo,
											   FGameplayAbilityActivationInfo const ActivationInfo,
											   FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding) // We ended the ability in the Super.
	{
		return;
	}

	CurrentShotIndex = 0;
	bWantsToFire = true;

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	ScheduleFireTrigger(ActivationInfo, AnimInstance);
}

void UGeoAutomaticFireAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
										  FGameplayAbilityActorInfo const* ActorInfo,
										  FGameplayAbilityActivationInfo const ActivationInfo,
										  bool bReplicateEndAbility, bool bWasCancelled)
{

	bWantsToFire = false;
	CurrentShotIndex = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGeoAutomaticFireAbility::ExecuteShot_Implementation()
{
	ensureMsgf(false, TEXT("Subclasses must override this function to play whatever needs to be done in the ability."));
	return true;
}

void UGeoAutomaticFireAbility::InputReleased(FGameplayAbilitySpecHandle const Handle,
											 FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayAbilityActivationInfo const ActivationInfo)
{
	bWantsToFire = false;

	if (IsActive() && !bIsAbilityEnding)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGeoAutomaticFireAbility::Fire()
{
	Super::Fire();

	if (!bWantsToFire || !IsActive())
	{
		return;
	}

	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		bWantsToFire = false;
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	bool const bShotSucceeded = ExecuteShot();
	CurrentShotIndex++;

	if (bShotSucceeded && bWantsToFire)
	{
		ScheduleFireTrigger(ActivationInfo, ActorInfo->GetAnimInstance());
	}
	else if (!bShotSucceeded)
	{
		bWantsToFire = false;
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGeoAutomaticFireAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
														FGameplayTag ApplicationTag)
{
	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	GetAbilitySystemComponentFromActorInfo()->ConsumeClientReplicatedTargetData(
		Handle, ActivationInfo.GetActivationPredictionKey());

	if (!bWantsToFire || !IsActive())
	{
		return;
	}

	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		bWantsToFire = false;
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (FGeoAbilityTargetData const* TargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0)))
	{
		StoredPayload.Origin = TargetData->Origin;
		StoredPayload.Yaw = TargetData->Yaw;
		StoredPayload.ServerSpawnTime = TargetData->ServerSpawnTime;
		StoredPayload.Seed = TargetData->Seed;
	}

	bool const bShotSucceeded = ExecuteShot();
	CurrentShotIndex++;

	if (!bShotSucceeded)
	{
		bWantsToFire = false;
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}
