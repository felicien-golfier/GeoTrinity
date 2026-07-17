// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoSweetSpotChargePassiveAbility.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoSweetSpotChargePassiveAbility::UGeoSweetSpotChargePassiveAbility()
{
	// Passives are server-owned: a client cancel request must never end the server's instance.
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSweetSpotChargePassiveAbility::GetGaugeRatio(UAbilitySystemComponent const& ASC) const
{
	float const HealCharge = ASC.GetNumericAttribute(UCharacterAttributeSet::GetHealChargeAttribute());
	return FMath::Clamp(HealCharge / HealRequiredForFullCharge, 0.f, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSweetSpotChargePassiveAbility::GetHealsToDamageMultiplier(float const SweetSpotPrecision) const
{
	return FMath::Lerp(EdgeDamageMultiplierBoost, CenterDamageMultiplierBoost, SweetSpotPrecision);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSweetSpotChargePassiveAbility::ConsumeGauge(UAbilitySystemComponent& ASC) const
{
	ASC.SetNumericAttributeBase(UCharacterAttributeSet::GetHealChargeAttribute(), 0.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSweetSpotChargePassiveAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,
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
	if (!ensureMsgf(SourceASC, TEXT("UGeoSweetSpotChargePassiveAbility: invalid ASC on activation")))
	{
		return;
	}

	if (GeoLib::IsServer(GetWorld()))
	{
		SourceASC->OnHealProvided.AddDynamic(this, &UGeoSweetSpotChargePassiveAbility::OnHealProvidedCallback);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSweetSpotChargePassiveAbility::EndAbility(FGameplayAbilitySpecHandle Handle,
												   FGameplayAbilityActorInfo const* ActorInfo,
												   FGameplayAbilityActivationInfo ActivationInfo,
												   bool bReplicateEndAbility, bool bWasCancelled)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (SourceASC && GeoLib::IsServer(GetWorld()))
	{
		SourceASC->OnHealProvided.RemoveDynamic(this, &UGeoSweetSpotChargePassiveAbility::OnHealProvidedCallback);
		ConsumeGauge(*SourceASC);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSweetSpotChargePassiveAbility::OnHealProvidedCallback(float HealDone)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoSweetSpotChargePassiveAbility: invalid ASC in OnHealProvidedCallback")))
	{
		return;
	}

	SourceASC->SetNumericAttributeBase(
		UCharacterAttributeSet::GetHealChargeAttribute(),
		FMath::Min(SourceASC->GetNumericAttribute(UCharacterAttributeSet::GetHealChargeAttribute()) + HealDone,
				   HealRequiredForFullCharge));
}
