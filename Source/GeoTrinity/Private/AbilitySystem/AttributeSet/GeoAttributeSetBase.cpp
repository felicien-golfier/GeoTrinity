// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GeoPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "System/GeoCombatStatsSubsystem.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGeoAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoAttributeSetBase, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoAttributeSetBase, Shield, COND_None, REPNOTIFY_Always);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::PostGameplayEffectExecute(FGameplayEffectModCallbackData const& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		float DamageToApply = GetIncomingDamage();
		SetIncomingDamage(0.f);
		if (DamageToApply > 0.f)
		{
			float const ShieldAbsorbed = FMath::Min(GetShield(), DamageToApply);
			SetShield(GetShield() - ShieldAbsorbed);
			DamageToApply -= ShieldAbsorbed;

			SetHealth(FMath::Clamp(GetHealth() - DamageToApply, 0.f, GetMaxHealth()));

			UGeoAbilitySystemComponent* SourceASC = Cast<UGeoAbilitySystemComponent>(
				Data.EffectSpec.GetEffectContext().GetOriginalInstigatorAbilitySystemComponent());
			if (IsValid(SourceASC))
			{
				FGameplayTag AbilityTag;
				if (FGameplayEffectContext const* Context = Data.EffectSpec.GetEffectContext().Get())
				{
					if (UGameplayAbility const* Ability = Context->GetAbilityInstance_NotReplicated())
					{
						AbilityTag = GeoASLib::GetAbilityTagFromAbility(*Ability);
					}
				}
				SourceASC->OnDamageDealt.Broadcast(DamageToApply, AbilityTag);
			}

			if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
			{
				AGeoPlayerState* TargetPlayerState = Cast<AGeoPlayerState>(GetOwningActor());
				AGeoPlayerState* SourcePlayerState =
					Cast<AGeoPlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
				CombatStats->ReportDamageDealt(SourcePlayerState, DamageToApply);
				CombatStats->ReportDamageReceived(TargetPlayerState, DamageToApply);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		float const HealToApply = GetIncomingHeal();
		SetIncomingHeal(0.f);
		if (HealToApply > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() + HealToApply, 0.f, GetMaxHealth()));

			FGeoGameplayEffectContext const* GeoContext =
				static_cast<FGeoGameplayEffectContext const*>(Data.EffectSpec.GetEffectContext().Get());
			UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(
				Data.EffectSpec.GetEffectContext().GetOriginalInstigatorAbilitySystemComponent());
			if (IsValid(ASC) && (!GeoContext || !GeoContext->IsSuppressHealProvided()))
			{
				ASC->OnHealProvided.Broadcast(HealToApply);
			}

			if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
			{
				AGeoPlayerState* SourcePlayerState =
					Cast<AGeoPlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
				if (!IsValid(SourcePlayerState))
				{
					SourcePlayerState = Cast<AGeoPlayerState>(Data.EffectSpec.GetEffectContext()
																  .GetOriginalInstigatorAbilitySystemComponent()
																  ->GetOwnerActor());
				}
				CombatStats->ReportHealingDealt(SourcePlayerState, HealToApply);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute()
			 && Data.EvaluatedData.ModifierOp == EGameplayModOp::Additive && Data.EvaluatedData.Magnitude != 0.f)
	{

		float const Magnitude = Data.EvaluatedData.Magnitude;
		AGeoPlayerState* SourcePlayerState = Cast<AGeoPlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
		if (!IsValid(SourcePlayerState))
		{
			SourcePlayerState = Cast<AGeoPlayerState>(
				Data.EffectSpec.GetEffectContext().GetOriginalInstigatorAbilitySystemComponent()->GetOwnerActor());
		}

		if (Magnitude > 0.f)
		{
			FGeoGameplayEffectContext const* GeoContext =
				static_cast<FGeoGameplayEffectContext const*>(Data.EffectSpec.GetEffectContext().Get());
			UGeoAbilitySystemComponent* ASC = Cast<UGeoAbilitySystemComponent>(
				Data.EffectSpec.GetEffectContext().GetOriginalInstigatorAbilitySystemComponent());
			if (IsValid(ASC) && (!GeoContext || !GeoContext->IsSuppressHealProvided()))
			{
				ASC->OnHealProvided.Broadcast(Magnitude);
			}

			if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
			{
				CombatStats->ReportHealingDealt(SourcePlayerState, Magnitude);
			}
		}
		else if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
		{
			AGeoPlayerState* TargetPlayerState = Cast<AGeoPlayerState>(GetOwningActor());
			CombatStats->ReportDamageDealt(SourcePlayerState, -Magnitude);
			CombatStats->ReportDamageReceived(TargetPlayerState, -Magnitude);
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
float UGeoAttributeSetBase::GetHealthRatio() const
{
	if (GetMaxHealth() > 0.f)
	{
		return GetHealth() / GetMaxHealth();
	}
	return 0.f;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::OnRep_Health(FGameplayAttributeData const& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoAttributeSetBase, Health, OldHealth);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::OnRep_MaxHealth(FGameplayAttributeData const& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGeoAttributeSetBase, MaxHealth, OldMaxHealth);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::OnRep_Shield(FGameplayAttributeData const& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, Shield, OldShield);
}
