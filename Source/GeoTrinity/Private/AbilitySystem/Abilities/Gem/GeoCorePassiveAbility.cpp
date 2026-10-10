// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Gem/GeoCorePassiveAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"

UGeoCorePassiveAbility::UGeoCorePassiveAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCorePassiveAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
											 FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayAbilityActivationInfo const ActivationInfo,
											 FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UGeoAbilitySystemComponent* HolderASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (IsActive()
		&& ensureMsgf(HolderASC, TEXT("%hs: %s has no UGeoAbilitySystemComponent"), __FUNCTION__, *GetName()))
	{
		BindEvent(*HolderASC);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoCorePassiveAbility::EndAbility(FGameplayAbilitySpecHandle const Handle,
										FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo const ActivationInfo,
										bool const bReplicateEndAbility, bool const bWasCancelled)
{
	if (UGeoAbilitySystemComponent* HolderASC = GetGeoAbilitySystemComponentFromActorInfo())
	{
		UnbindEvent(*HolderASC);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoCorePassiveAbility::TryTrigger()
{
	float const Now = GetWorld()->GetTimeSeconds();
	bool const bTriggered = Now - LastTriggerTime >= CooldownSeconds && FMath::FRand() < TriggerChance;
	if (bTriggered)
	{
		LastTriggerTime = Now;
	}

	return bTriggered;
}
