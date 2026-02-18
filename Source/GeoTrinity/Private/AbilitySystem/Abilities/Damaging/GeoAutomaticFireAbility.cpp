// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Damaging/GeoAutomaticFireAbility.h"

#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Characters/PlayableCharacter.h"
#include "Settings/GameDataSettings.h"
#include "Tool/GameplayLibrary.h"

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
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

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

	CurrentShotIndex = 0;
	bWantsToFire = true;

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	ScheduleFireTrigger(ActivationInfo, AnimInstance, StoredPayload.ServerSpawnTime);
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

	UpdatePayload();

	// Execute the shot - subclasses define what happens
	bool const bShotSucceeded = ExecuteShot();

	// Increment shot counter
	CurrentShotIndex++;

	if (bShotSucceeded && bWantsToFire && IsActive())
	{
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
		ScheduleFireTrigger(ActivationInfo, AnimInstance, GameplayLibrary::GetServerTime(GetWorld()));
	}
	else if (!bShotSucceeded)
	{
		bWantsToFire = false;
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGeoAutomaticFireAbility::UpdatePayload()
{
	StoredPayload.Seed += CurrentShotIndex;

	AActor* const Avatar = GetAvatarActorFromActorInfo();
	ensureMsgf(IsValid(Avatar), TEXT("Avatar Actor from actor info is invalid!"));

	StoredPayload.Origin = FVector2D(Avatar->GetActorLocation());
	StoredPayload.Yaw = GameplayLibrary::GetYawWithNetworkDelay(Avatar, CachedNetworkDelay);

	if (Avatar->HasAuthority() && GetDefault<UGameDataSettings>()->bNetworkDelayCompensation)
	{
		if (APlayableCharacter const* PlayableCharacter = Cast<APlayableCharacter>(Avatar))
		{
			StoredPayload.Yaw += PlayableCharacter->GetYawVelocity() * CachedNetworkDelay;
		}
	}
}

float UGeoAutomaticFireAbility::GetShotServerTime(int32 ShotIndex) const
{
	return StoredPayload.ServerSpawnTime + (static_cast<float>(ShotIndex) * FireRate);
}
