// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

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
