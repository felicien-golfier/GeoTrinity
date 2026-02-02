// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Actor/Projectile/GeoProjectile.h"
#include "System/GeoActorPoolingSubsystem.h"

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
	if (FGeoAbilityTargetData const* TargetData =
			static_cast<FGeoAbilityTargetData const*>(TriggerEventData->TargetData.Get(0)))
	{
		StoredPayload = CreateAbilityPayload(GetOwningActorFromActorInfo(), Instigator, TargetData->Origin,
											 TargetData->Yaw, TargetData->ServerSpawnTime, TargetData->Seed);
		InitialServerSpawnTime = TargetData->ServerSpawnTime;
		InitialSeed = TargetData->Seed;
	}
	else
	{
		StoredPayload = CreateAbilityPayload(GetOwningActorFromActorInfo(), Instigator, Instigator->GetTransform());
		InitialServerSpawnTime = StoredPayload.ServerSpawnTime;
		InitialSeed = StoredPayload.Seed;
	}

	// Reset firing state
	CurrentShotIndex = 0;
	bWantsToFire = true;

	// Setup WaitInputRelease task for GAS-compatible input release detection
	// This task properly handles the input release event through GAS infrastructure
	UAbilityTask_WaitInputRelease* WaitInputReleaseTask =
		UAbilityTask_WaitInputRelease::WaitInputRelease(this, true /*bTestAlreadyReleased*/);
	if (WaitInputReleaseTask)
	{
		WaitInputReleaseTask->OnRelease.AddDynamic(this, &UGeoAutomaticFireAbility::OnInputReleased);
		WaitInputReleaseTask->ReadyForActivation();
	}

	// Start firing
	if (bFireImmediately)
	{
		// Fire first shot immediately
		FireShot();
	}
	else
	{
		// Schedule first shot after interval
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(FireTimerHandle, this, &UGeoAutomaticFireAbility::FireShot, FireInterval,
											  false);
		}
	}
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
	InitialServerSpawnTime = 0.f;
	InitialSeed = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGeoAutomaticFireAbility::InputReleased(FGameplayAbilitySpecHandle const Handle,
											 FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayAbilityActivationInfo const ActivationInfo)
{
	// Direct input release callback from ASC
	// Mark that we want to stop firing - OnInputReleased will handle EndAbility via the task
	bWantsToFire = false;
}

void UGeoAutomaticFireAbility::OnInputReleased(float TimeHeld)
{
	// Called from WaitInputRelease task
	// This provides GAS-compatible input release handling with proper prediction
	bWantsToFire = false;

	// Only end if still active (prevents double-end)
	if (IsActive() && !bIsAbilityEnding)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UGeoAutomaticFireAbility::FireShot()
{
	// Check if we should continue firing
	if (!bWantsToFire || !IsActive())
	{
		return;
	}

	// Update payload with deterministic server time for this shot
	// This ensures client and server spawn projectiles at the same logical time
	StoredPayload.ServerSpawnTime = GetShotServerTime(CurrentShotIndex);

	// Use deterministic seed variation per shot
	// Each shot gets a predictable seed based on initial seed and shot index
	StoredPayload.Seed = InitialSeed + CurrentShotIndex;

	// Optionally update position from current avatar location (for moving characters)
	// When disabled, uses initial position for better network sync but less responsiveness
	if (bUpdatePositionPerShot)
	{
		if (AActor const* Avatar = GetAvatarActorFromActorInfo())
		{
			StoredPayload.Origin = FVector2D(Avatar->GetActorLocation());
			StoredPayload.Yaw = Avatar->GetActorRotation().Yaw;
		}
	}

	// Spawn projectiles using parent class logic
	SpawnProjectilesUsingTarget();

	// Increment shot counter
	CurrentShotIndex++;

	// Optionally commit cost per shot (for ammo/energy systems)
	if (!bCommitOnceOnly)
	{
		FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
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
	}

	// Schedule next shot
	if (bWantsToFire && IsActive())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(FireTimerHandle, this, &UGeoAutomaticFireAbility::FireShot, FireInterval,
											  false);
		}
	}
}

float UGeoAutomaticFireAbility::GetShotServerTime(int32 ShotIndex) const
{
	// Deterministic shot timing: each shot's server time is predictable
	// Both client and server calculate the same values, ensuring sync
	return InitialServerSpawnTime + (static_cast<float>(ShotIndex) * FireInterval);
}
