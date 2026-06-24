// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoHealReturnPassiveAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Tool/UGeoGameplayLibrary.h"

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

	if (GeoLib::IsServer(GetWorld()))
	{
		SourceASC->OnHealProvided.AddDynamic(this, &UGeoHealReturnPassiveAbility::OnHealProvidedCallback);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealReturnPassiveAbility::EndAbility(FGameplayAbilitySpecHandle Handle,
											  FGameplayAbilityActorInfo const* ActorInfo,
											  FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
											  bool bWasCancelled)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (SourceASC && GeoLib::IsServer(GetWorld()))
	{
		SourceASC->OnHealProvided.RemoveDynamic(this, &UGeoHealReturnPassiveAbility::OnHealProvidedCallback);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealReturnPassiveAbility::OnHealProvidedCallback(float HealDone)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoHealReturnPassiveAbility: invalid ASC in OnHealProvidedCallback")))
	{
		return;
	}

	float const Health = SourceASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());
	float const MaxHealth = SourceASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute());
	if (Health == MaxHealth)
	{
		return;
	}

	FHealEffectData HealEffect;
	HealEffect.HealAmount = FScalableFloat(HealDone * SelfHealPercent);
	HealEffect.bSuppressHealProvided = true;
	HealEffect.bLimitGameplayCue = true;

	UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffect, SourceASC, SourceASC, GetAbilityLevel(),
													StoredPayload.Seed, StoredPayload.AbilityTag);
}
