// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"

UGeoAutomaticFireAbility::UGeoAutomaticFireAbility()
{
	// Following Lyra's pattern: don't re-trigger while already active
	bRetriggerInstancedAbility = false;

	// Need instanced ability to maintain firing state
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Replicate to server for authoritative validation
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UGeoAutomaticFireAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											   FGameplayAbilityActorInfo const* ActorInfo,
											   FGameplayAbilityActivationInfo const ActivationInfo,
											   FGameplayEventData const* TriggerEventData)
{
	// Commit ability (cost/cooldown) at start
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Extract initial state from trigger event data for deterministic sync
	AActor* Instigator = GetAvatarActorFromActorInfo();
	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		if (FGeoAbilityTargetData const* TargetData =
				static_cast<FGeoAbilityTargetData const*>(TriggerEventData->TargetData.Get(0)))
		{
			StoredPayload = CreateAbilityPayload(GetOwningActorFromActorInfo(), Instigator, TargetData->Origin,
												 TargetData->Yaw, TargetData->ServerSpawnTime, TargetData->Seed);
		}
	}
	else if (Instigator)
	{
		StoredPayload = CreateAbilityPayload(GetOwningActorFromActorInfo(), Instigator, Instigator->GetTransform());
	}

	// Reset firing state
	CurrentShotIndex = 0;
	bWantsToFire = true;

	FireShot();
}

void UGeoAutomaticFireAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
										  FGameplayAbilityActorInfo const* ActorInfo,
										  FGameplayAbilityActivationInfo const ActivationInfo,
										  bool bReplicateEndAbility, bool bWasCancelled)
{
	// Stop firing
	bWantsToFire = false;

	// Clear the fire timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}

	// Reset state for next activation
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

void UGeoAutomaticFireAbility::FireShot()
{
	// Check if we should continue firing
	if (!bWantsToFire || !IsActive())
	{
		return;
	}

	FGameplayAbilitySpecHandle const Handle = GetCurrentAbilitySpecHandle();
	FGameplayAbilityActorInfo const* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo const ActivationInfo = GetCurrentActivationInfo();

	// Only commit cost, not cooldown (cooldown applied at end)
	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		// Out of resources, stop firing
		bWantsToFire = false;
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Optionally update position from current avatar location (for moving characters)
	// When disabled, uses initial position for better network sync but less responsiveness
	if (bUpdatePositionPerShot)
	{
		UpdatePayloadFromAvatar();
	}

	// Use deterministic seed variation per shot
	// Each shot gets a predictable seed based on initial seed and shot index
	StoredPayload.Seed += CurrentShotIndex;

	// Execute the shot - subclasses define what happens
	bool const bShotSucceeded = ExecuteShot();

	// Increment shot counter
	CurrentShotIndex++;

	// Schedule next shot if shot succeeded and we still want to fire
	if (bShotSucceeded && bWantsToFire && IsActive())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(FireTimerHandle, this, &UGeoAutomaticFireAbility::FireShot, FireInterval,
											  false);
		}
	}
	else if (!bShotSucceeded)
	{
		// Shot failed, end the ability
		bWantsToFire = false;
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGeoAutomaticFireAbility::UpdatePayloadFromAvatar()
{
	if (AActor const* Avatar = GetAvatarActorFromActorInfo())
	{
		StoredPayload.Origin = FVector2D(Avatar->GetActorLocation());
		StoredPayload.Yaw = Avatar->GetActorRotation().Yaw;
	}
}

float UGeoAutomaticFireAbility::GetShotServerTime(int32 ShotIndex) const
{
	// Deterministic shot timing: each shot's server time is predictable
	// Both client and server calculate the same values, ensuring sync
	return StoredPayload.ServerSpawnTime + (static_cast<float>(ShotIndex) * FireInterval);
}
