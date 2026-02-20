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
	if (ActorInfo->IsLocallyControlledPlayer())
	{
		ScheduleFireTrigger(ActivationInfo, AnimInstance);
	}
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
}

void UGeoAutomaticFireAbility::Fire()
{
	// Don't call super, as we don't want to send the data in every situations.

	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	if (!IsActive() || bIsAbilityEnding)
	{
		return;
	}

	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AActor const* Avatar = ActorInfo->AvatarActor.Get();
	StoredPayload.Origin = FVector2D(Avatar->GetActorLocation());
	StoredPayload.Yaw = Avatar->GetActorRotation().Yaw;
	StoredPayload.ServerSpawnTime = UGameplayLibrary::GetServerTime(GetWorld(), true);

	bool const bShotSucceeded = ExecuteShot();
	StoredPayload.Seed += CurrentShotIndex;
	CurrentShotIndex++;

	if (bShotSucceeded)
	{
		SendFireDataToServer(); // Call RPC only when we actually shoot !
	}

	if (bWantsToFire)
	{
		ScheduleFireTrigger(ActivationInfo, ActorInfo->GetAnimInstance());
	}
	else
	{
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

	if (!IsActive() || bIsAbilityEnding)
	{
		return;
	}

	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FGeoAbilityTargetData const* TargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));

	// Update payload with the information, as we read from it to spawn projectile
	StoredPayload.Origin = TargetData->Origin;
	StoredPayload.Yaw = TargetData->Yaw;
	StoredPayload.ServerSpawnTime = TargetData->ServerSpawnTime;

	ExecuteShot();
	StoredPayload.Seed += CurrentShotIndex; // don't use target data, to avoid client abuse. (lol)
	CurrentShotIndex++;
}
