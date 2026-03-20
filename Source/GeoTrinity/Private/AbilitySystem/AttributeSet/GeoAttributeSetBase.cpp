// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GeoPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "System/GeoCombatStatsSubsystem.h"

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::PostGameplayEffectExecute(FGameplayEffectModCallbackData const& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		float const DamageToApply = GetIncomingDamage();
		SetIncomingDamage(0.f);
		if (DamageToApply > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - DamageToApply, 0.f, GetMaxHealth()));
			if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
			{
				AGeoPlayerState* TargetPlayerState = Cast<AGeoPlayerState>(GetOwningActor());
				AGeoPlayerState* SourcePlayerState = Cast<AGeoPlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
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
			if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
			{
				AGeoPlayerState* SourcePlayerState = Cast<AGeoPlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
				CombatStats->ReportHealingDealt(SourcePlayerState, HealToApply);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute() &&
			 Data.EvaluatedData.ModifierOp == EGameplayModOp::Additive && Data.EvaluatedData.Magnitude != 0.f)
	{
		if (UGeoCombatStatsSubsystem* CombatStats = GetWorld()->GetSubsystem<UGeoCombatStatsSubsystem>())
		{
			float const Magnitude = Data.EvaluatedData.Magnitude;
			AGeoPlayerState* SourcePlayerState = Cast<AGeoPlayerState>(Data.EffectSpec.GetEffectContext().GetInstigator());
			if (Magnitude > 0.f)
			{
				CombatStats->ReportHealingDealt(SourcePlayerState, Magnitude);
			}
			else
			{
				AGeoPlayerState* TargetPlayerState = Cast<AGeoPlayerState>(GetOwningActor());
				CombatStats->ReportDamageDealt(SourcePlayerState, -Magnitude);
				CombatStats->ReportDamageReceived(TargetPlayerState, -Magnitude);
			}
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------
void UGeoAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGeoAttributeSetBase, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGeoAttributeSetBase, MaxHealth, COND_None, REPNOTIFY_Always);
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
