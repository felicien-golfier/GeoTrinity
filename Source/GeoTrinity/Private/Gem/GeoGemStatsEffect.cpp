// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Gem/GeoGemStatsEffect.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoGemAttributeSet.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Gem/GeoGemCatalog.h"

UGeoGemStatsEffect::UGeoGemStatsEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	TArray<FGameplayAttribute> GemAttributes = {
		UCharacterAttributeSet::GetMaxHealthAttribute(),
		UCharacterAttributeSet::GetMaxAmmoAttribute(),
		UCharacterAttributeSet::GetDamageMultiplierAttribute(),
		UCharacterAttributeSet::GetDamageReductionAttribute(),
		UCharacterAttributeSet::GetAppliedHealBoostAttribute(),
		UCharacterAttributeSet::GetReceivedHealBoostAttribute(),
		UCharacterAttributeSet::GetMovementSpeedMultiplierAttribute(),
	};
	UAttributeSet::GetAttributesFromSetClass(UGeoGemAttributeSet::StaticClass(), GemAttributes);
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
	FGameplayTagContainer GrantedTags;
	auto AddCopies = [&SummedMagnitudes](FGeoGemInfo const& Gem, float const Copies)
	{
		if (Gem.Attribute.IsValid())
		{
			ensureMsgf(Supports(Gem), TEXT("%hs: gem %s raises %s, which UGeoGemStatsEffect does not list"),
					   __FUNCTION__, *Gem.Id.ToString(), *Gem.Attribute.GetName());
			SummedMagnitudes.FindOrAdd(GetSetByCallerName(Gem.Attribute, Gem.Operation)) +=
				Gem.MagnitudePerGem * Copies;
		}
	};

	for (FName const GemId : Loadout.Sockets)
	{
		if (FGeoGemInfo const* Gem = Catalog.Find(GemId))
		{
			if (Gem->GrantedTag.IsValid())
			{
				GrantedTags.AddTag(Gem->GrantedTag);
			}

			if (Gem->GrantedTag == FGeoGameplayTags::Get().Gem_Core_Critical)
			{
				SummedMagnitudes.FindOrAdd(GetSetByCallerName(UGeoGemAttributeSet::GetCritChanceAttribute(),
															  EGeoGemOperation::Add)) += CriticalCoreCritChance;
			}

			AddCopies(*Gem, 1.f);
		}
	}

	FGeoGemInfo const* EchoGem = Catalog.Find(Loadout.Echo);
	TOptional<EGeoGemTier> const EchoTier = Catalog.FindTier(Loadout.Echo);
	if (EchoGem && EchoTier)
	{
		AddCopies(*EchoGem, Catalog.GetEchoCopies(*EchoTier, Loadout.EchoLevel));
	}

	FGameplayEffectSpecHandle SpecHandle = ASC.MakeOutgoingSpec(StaticClass(), 1.f, ASC.MakeEffectContext());
	for (FGameplayModifierInfo const& Modifier : Effect->Modifiers)
	{
		FName const SetByCallerName = Modifier.ModifierMagnitude.GetSetByCallerFloat().DataName;
		float const Neutral = Modifier.ModifierOp == EGameplayModOp::MultiplyAdditive ? 1.f : 0.f;
		SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerName, Neutral + SummedMagnitudes.FindRef(SetByCallerName));
	}
	SpecHandle.Data->DynamicGrantedTags = GrantedTags;
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
	TCHAR const* OperationName = Operation == EGeoGemOperation::Percent ? TEXT("Percent") : TEXT("Add");
	return *FString::Printf(TEXT("Gem.%s.%s"), *Attribute.GetName(), OperationName);
}
