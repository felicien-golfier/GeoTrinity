// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoAutomaticFireAbility::UGeoAutomaticFireAbility()
{
	bRetriggerInstancedAbility = false;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	CommitBehaviour = ECommitBehaviour::DoNotAutoCommit;
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

void UGeoAutomaticFireAbility::GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle,
																   FGameplayAbilityActorInfo const* ActorInfo,
																   float& TimeRemaining, float& CooldownDuration) const
{
	if (FireTriggerTimerHandle.IsValid() && GetWorld())
	{
		TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(FireTriggerTimerHandle);
		CooldownDuration = GetFireDelay();
		return;
	}

	Super::GetCooldownTimeRemainingAndDuration(Handle, ActorInfo, TimeRemaining, CooldownDuration);
}

bool UGeoAutomaticFireAbility::ExecuteShot_Implementation()
{
	ensureMsgf(false, TEXT("Subclasses must override this function to play whatever needs to be done in the ability."));
	return true;
}

int32& UGeoAutomaticFireAbility::GetFireSectionIndex(UGeoAbilitySystemComponent* ASC, UAnimInstance const* AnimInstance)
{
	int32& FireSectionIndex = ASC->GetFireSectionIndex(GetAbilityTag());

	// For auto-fire, the montage loops continuously — always advance the index.
	// The base class resets on ability end, so the counter naturally starts from 0 again next activation.
	if (AnimMontage->IsValidSectionName(FName(*FString::Printf(TEXT("%s1"), *GeoASLib::SectionFireString))))
	{
		++FireSectionIndex;
	}
	else
	{
		FireSectionIndex = 0; // When the montage has no Fire loop system.
	}
	return FireSectionIndex;
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

	// The server fires exclusively via OnFireTargetDataReceived (one shot per client packet).
	// Running the fire loop server-side too would spawn a duplicate projectile for every shot.
	if (!ActorInfo->IsLocallyControlledPlayer())
	{
		return;
	}

	if (!IsActive() || bIsAbilityEnding)
	{
		return;
	}

	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	if (!CheckCost(Handle, ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
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
			CommitAbility(Handle, ActorInfo, ActivationInfo);

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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGeoAbilityTargetData const* TargetData = static_cast<FGeoAbilityTargetData const*>(DataHandle.Get(0));
	if (!ensureMsgf(TargetData,
					TEXT("GeoAutomaticFireAbility: No FGeoAbilityTargetData in DataHandle — shot skipped on server.")))
	{
		return;
	}

	// Update payload with the information, as we read from it to spawn projectile
	StoredPayload.Origin = TargetData->Origin;
	StoredPayload.Yaw = TargetData->Yaw;
	StoredPayload.ServerSpawnTime = TargetData->ServerSpawnTime;

	ExecuteShot();

	// On a listen host the locally-controlled player and the server share one ASC, and Fire() already committed the
	// cost on this machine — committing again here would charge the cost twice (e.g. ammo dropping 2 per shot for the
	// host only). Only the server-for-a-remote-client path must commit here.
	if (!ActorInfo->IsLocallyControlledPlayer())
	{
		CommitAbility(Handle, ActorInfo, ActivationInfo);
	}

	// Seed is incremented server-side using the authoritative shot counter rather than reading it from
	// the client's target data, so a cheating client cannot fabricate a seed that bypasses status-proc checks.
	StoredPayload.Seed += CurrentShotIndex;
	CurrentShotIndex++;
}
