// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemStatsEffect.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Gem/GeoGemCatalog.h"

UGeoGemStatsEffect::UGeoGemStatsEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayAttribute const GemAttributes[] = {
		UCharacterAttributeSet::GetMaxHealthAttribute(),
		UCharacterAttributeSet::GetMaxAmmoAttribute(),
		UCharacterAttributeSet::GetDamageMultiplierAttribute(),
		UCharacterAttributeSet::GetDamageReductionAttribute(),
		UCharacterAttributeSet::GetAppliedHealBoostAttribute(),
		UCharacterAttributeSet::GetReceivedHealBoostAttribute(),
		UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute(),
	};
	for (FGameplayAttribute const& Attribute : GemAttributes)
	{
		for (EGeoGemOperation const Operation : {EGeoGemOperation::Add, EGeoGemOperation::Percent})
		{
			FSetByCallerFloat SetByCaller;
			SetByCaller.DataName = GetSetByCallerName(Attribute, Operation);

			FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp =
				Operation == EGeoGemOperation::Add ? EGameplayModOp::AddBase : EGameplayModOp::MultiplyAdditive;
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		}
	}
}

FGameplayEffectSpecHandle UGeoGemStatsEffect::MakeSpec(UAbilitySystemComponent const& ASC,
													   UGeoGemCatalog const& Catalog, FGeoGemLoadout const& Loadout)
{
	UGeoGemStatsEffect const* Effect = GetDefault<UGeoGemStatsEffect>();

	TMap<FName, float> SummedMagnitudes;
	for (FName const GemId : Loadout.Sockets)
	{
		FGeoGemInfo const* Gem = Catalog.Find(GemId);
		if (Gem && Gem->Attribute.IsValid())
		{
			ensureMsgf(Supports(*Gem), TEXT("%hs: gem %s raises %s, which UGeoGemStatsEffect does not list"),
					   __FUNCTION__, *GemId.ToString(), *Gem->Attribute.GetName());
			SummedMagnitudes.FindOrAdd(GetSetByCallerName(Gem->Attribute, Gem->Operation)) += Gem->MagnitudePerGem;
		}
	}

	FGameplayEffectSpecHandle SpecHandle = ASC.MakeOutgoingSpec(StaticClass(), 1.f, ASC.MakeEffectContext());
	for (FGameplayModifierInfo const& Modifier : Effect->Modifiers)
	{
		FName const SetByCallerName = Modifier.ModifierMagnitude.GetSetByCallerFloat().DataName;
		float const Neutral = Modifier.ModifierOp == EGameplayModOp::MultiplyAdditive ? 1.f : 0.f;
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerName, Neutral + SummedMagnitudes.FindRef(SetByCallerName));
	}
	return SpecHandle;
}

bool UGeoGemStatsEffect::Supports(FGeoGemInfo const& Gem)
{
	FName const SetByCallerName = GetSetByCallerName(Gem.Attribute, Gem.Operation);
	return !Gem.Attribute.IsValid()
		|| GetDefault<UGeoGemStatsEffect>()->Modifiers.ContainsByPredicate(
			   [SetByCallerName](FGameplayModifierInfo const& Modifier)
			   {
				   return Modifier.ModifierMagnitude.GetSetByCallerFloat().DataName == SetByCallerName;
			   });
}

FName UGeoGemStatsEffect::GetSetByCallerName(FGameplayAttribute const& Attribute, EGeoGemOperation const Operation)
{
	TCHAR const* OperationName = Operation == EGeoGemOperation::Add ? TEXT("Add") : TEXT("Percent");
	return *FString::Printf(TEXT("Gem.%s.%s"), *Attribute.GetName(), OperationName);
}
