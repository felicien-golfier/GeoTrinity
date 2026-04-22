// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoAutomaticFireAbility::UGeoAutomaticFireAbility()
{
	bRetriggerInstancedAbility = false;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	bCommitAtActivate = false;
}

void UGeoAutomaticFireAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											   FGameplayAbilityActorInfo const* ActorInfo,
											   FGameplayAbilityActivationInfo const ActivationInfo,
											   FGameplayEventData const* TriggerEventData)
{
	CurrentShotIndex = 0;
	bWantsToFire = true;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGeoAutomaticFireAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
										  FGameplayAbilityActorInfo const* ActorInfo,
										  FGameplayAbilityActivationInfo const ActivationInfo,
										  bool bReplicateEndAbility, bool bWasCancelled)
{

	bWantsToFire = false;
	CurrentShotIndex = 0;
	if (AnimMontage)
	{
		GetGeoAbilitySystemComponentFromActorInfo()->GetFireSectionIndex(GetAbilityTag()) = -1;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGeoAutomaticFireAbility::ExecuteShot_Implementation()
{
	ensureMsgf(false, TEXT("Subclasses must override this function to play whatever needs to be done in the ability."));
	return true;
}

void UGeoAutomaticFireAbility::InitFireSectionIndex(UAnimInstance* AnimInstance, int32& FireSectionIndex)
{
	// Reset FireSectionIndex only at the ability end.
	++FireSectionIndex;
}

void UGeoAutomaticFireAbility::InputReleased(FGameplayAbilitySpecHandle const Handle,
											 FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayAbilityActivationInfo const ActivationInfo)
{
	bWantsToFire = false;
}

void UGeoAutomaticFireAbility::Fire(FGeoAbilityTargetData const& AbilityTargetData)
{
	// Don't call super, as we don't want to send the data in every situations.

	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	if (!IsActive() || bIsAbilityEnding)
	{
		return;
	}

	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	if (!CheckCost(Handle, ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	StoredPayload.Origin = FVector2D(AbilityTargetData.Origin);
	StoredPayload.Yaw = AbilityTargetData.Yaw;
	StoredPayload.ServerSpawnTime = AbilityTargetData.ServerSpawnTime;

	bool const bShotSucceeded = ExecuteShot();
	StoredPayload.Seed += CurrentShotIndex;
	CurrentShotIndex++;

	if (bShotSucceeded)
	{
		if (ActorInfo->IsLocallyControlledPlayer())
		{
			CommitAbilityCost(Handle, ActorInfo, ActivationInfo);

			if (FireCameraShakeClass)
			{
				GeoLib::TriggerCameraShake(this, FireCameraShakeClass);
			}

			if (RecoilDistance > 0.f)
			{
				if (UGeoGameFeelComponent* GameFeel =
						GetAvatarActorFromActorInfo()->FindComponentByClass<UGeoGameFeelComponent>())
				{
					GameFeel->ApplyRecoil(RecoilDistance);
				}
			}

			if (FireGameplayCueTag.IsValid())
			{
				FGameplayCueParameters CueParams;
				CueParams.Location = FVector(StoredPayload.Origin, ArbitraryCharacterZ);
				CueParams.Instigator = StoredPayload.Instigator;
				CueParams.AbilityLevel = StoredPayload.AbilityLevel;
				CueParams.RawMagnitude = StoredPayload.Seed;
				CueParams.Normal = FRotator(0, StoredPayload.Yaw, 0).Vector();
				GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(FireGameplayCueTag, CueParams);
			}
		}
		SendFireDataToServer(AbilityTargetData);
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
	// Server only function

	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	GetAbilitySystemComponentFromActorInfo()->ConsumeClientReplicatedTargetData(
		Handle, ActivationInfo.GetActivationPredictionKey());

	if (!IsActive() || bIsAbilityEnding)
	{
		return;
	}

	if (!CheckCost(Handle, ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FGeoAbilityTargetData const* TargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	if (!ensureMsgf(TargetData, TEXT("GeoAutomaticFireAbility: No FGeoAbilityTargetData in DataHandle — shot skipped on server.")))
	{
		return;
	}

	// Update payload with the information, as we read from it to spawn projectile
	StoredPayload.Origin = TargetData->Origin;
	StoredPayload.Yaw = TargetData->Yaw;
	StoredPayload.ServerSpawnTime = TargetData->ServerSpawnTime;

	ExecuteShot();
	CommitAbilityCost(Handle, ActorInfo, ActivationInfo);

	StoredPayload.Seed += CurrentShotIndex; // don't use target data, to avoid client abuse. (lol)
	CurrentShotIndex++;
}
