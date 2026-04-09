// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoHealReturnPassiveAbility.h"

#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealReturnPassiveAbility::OnAvatarSet(FGameplayAbilityActorInfo const* ActorInfo,
											   FGameplayAbilitySpec const& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealReturnPassiveAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,
												   FGameplayAbilityActorInfo const* ActorInfo,
												   FGameplayAbilityActivationInfo ActivationInfo,
												   FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoHealReturnPassiveAbility: invalid ASC on activation")))
	{
		return;
	}

	SourceASC->OnHealProvided.AddDynamic(this, &UGeoHealReturnPassiveAbility::OnHealProvidedCallback);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealReturnPassiveAbility::EndAbility(FGameplayAbilitySpecHandle Handle,
											  FGameplayAbilityActorInfo const* ActorInfo,
											  FGameplayAbilityActivationInfo ActivationInfo,
											  bool bReplicateEndAbility, bool bWasCancelled)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (SourceASC)
	{
		SourceASC->OnHealProvided.RemoveDynamic(this, &UGeoHealReturnPassiveAbility::OnHealProvidedCallback);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealReturnPassiveAbility::OnHealProvidedCallback(float HealDone)
{
	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoHealReturnPassiveAbility: invalid ASC in OnHealProvidedCallback")))
	{
		return;
	}

	FHealEffectData HealEffect;
	HealEffect.HealAmount = FScalableFloat(HealDone * SelfHealPercent);
	HealEffect.bSuppressHealProvided = true;
	UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffect, SourceASC, SourceASC, GetAbilityLevel(),
													StoredPayload.Seed);
}
