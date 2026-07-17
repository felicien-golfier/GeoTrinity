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
UGeoSweetSpotChargePassiveAbility const* UGeoSweetSpotChargePassiveAbility::FindOnASC(UAbilitySystemComponent const& ASC)
{
	for (FGameplayAbilitySpec const& Spec : ASC.GetActivatableAbilities())
	{
		if (UGeoSweetSpotChargePassiveAbility const* Passive = Cast<UGeoSweetSpotChargePassiveAbility>(Spec.Ability))
		{
			return Passive;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSweetSpotChargePassiveAbility::GetGaugeRatio(UAbilitySystemComponent const& ASC) const
{
	float const StartTime = ASC.GetNumericAttribute(UCharacterAttributeSet::GetHealChargeStartTimeAttribute());
	if (StartTime <= 0.f)
	{
		return 0.f;
	}
	float const Elapsed = ASC.GetActiveGameplayEffects().GetServerWorldTime() - StartTime;
	return FMath::Clamp(Elapsed / ChargeDuration, 0.f, 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
float UGeoSweetSpotChargePassiveAbility::GetBoostDamage(UAbilitySystemComponent const& ASC) const
{
	return HealToDamageRatio * ASC.GetNumericAttribute(UCharacterAttributeSet::GetHealChargeAttribute());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoSweetSpotChargePassiveAbility::ConsumeGauge(UAbilitySystemComponent& ASC) const
{
	ASC.SetNumericAttributeBase(UCharacterAttributeSet::GetHealChargeAttribute(), 0.f);
	ASC.SetNumericAttributeBase(UCharacterAttributeSet::GetHealChargeStartTimeAttribute(), 0.f);
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

	float const Now = SourceASC->GetActiveGameplayEffects().GetServerWorldTime();
	float const StartTime = SourceASC->GetNumericAttribute(UCharacterAttributeSet::GetHealChargeStartTimeAttribute());
	if (StartTime <= 0.f)
	{
		SourceASC->SetNumericAttributeBase(UCharacterAttributeSet::GetHealChargeStartTimeAttribute(), Now);
	}
	else if (Now - StartTime >= ChargeDuration)
	{
		// Window elapsed: the gauge holds its recorded healing until a sweet-spot release consumes it.
		return;
	}

	SourceASC->SetNumericAttributeBase(
		UCharacterAttributeSet::GetHealChargeAttribute(),
		SourceASC->GetNumericAttribute(UCharacterAttributeSet::GetHealChargeAttribute()) + HealDone);
}
