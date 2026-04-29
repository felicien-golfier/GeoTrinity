// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoHealReturnPassiveAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "GameFramework/Character.h"
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

	ACharacter const* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UGeoGameFeelComponent* GameFeelComponent =
		Character ? Character->FindComponentByClass<UGeoGameFeelComponent>() : nullptr;
	if (ensureMsgf(GameFeelComponent, TEXT("UGeoHealReturnPassiveAbility: avatar has no GeoGameFeelComponent")))
	{
		HealEffect.bSuppressGameplayCue = !GameFeelComponent->IsHealCueAvailable();
	}

	UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffect, SourceASC, SourceASC, GetAbilityLevel(),
													StoredPayload.Seed);
}
