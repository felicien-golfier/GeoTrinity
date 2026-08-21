#include "AbilitySystem/Data/EffectData.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Settings/GameDataSettings.h"

FString MarkUpValue(FString const& Value, FDescriptionFormat const& Format)
{
	return Format.bRichTextValues ? FString::Printf(TEXT("<Value>%s</>"), *Value) : Value;
}

FString FormatValueRange(float Min, float Max, FDescriptionFormat const& Format)
{
	TCHAR const* Suffix = TEXT("");
	if (Format.ValueFormat != EValueFormat::Plain)
	{
		float const Base = Format.ValueFormat == EValueFormat::BonusPercent ? 1.f : 0.f;
		Min = (Min - Base) * 100.f;
		Max = (Max - Base) * 100.f;
		Suffix = TEXT("%");
	}
	return MarkUpValue(FMath::IsNearlyEqual(Min, Max) ? FString::Printf(TEXT("%g%s"), Min, Suffix)
													  : FString::Printf(TEXT("%g-%g%s"), Min, Max, Suffix),
					   Format);
}

FString FormatScalableRange(FScalableFloat const& Scalable, FDescriptionFormat const& Format)
{
	return FormatValueRange(Scalable.GetValueAtLevel(Format.MinLevel()), Scalable.GetValueAtLevel(Format.MaxLevel()),
							Format);
}

FString GetTagLeafName(FGameplayTag const& Tag)
{
	FString LeafName = Tag.GetTagName().ToString();
	int32 LastDotIndex = INDEX_NONE;
	LeafName.FindLastChar(TEXT('.'), LastDotIndex);
	return LastDotIndex == INDEX_NONE ? LeafName : LeafName.Mid(LastDotIndex + 1);
}

void FEffectData::UpdateContextHandle(FGeoGameplayEffectContext*, int32, FGameplayTag) const
{
	// By default does nothing. Override for your needs
}

FString FEffectData::GetDescriptionLine(FDescriptionFormat const& /*Format*/) const
{
	return FString();
}

FActiveGameplayEffectHandle FEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
													 UAbilitySystemComponent* SourceASC,
													 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
													 int32 Seed) const
{
	// By default does nothing. Override for your needs
	return FActiveGameplayEffectHandle();
}

FActiveGameplayEffectHandle FGameplayEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
															 UAbilitySystemComponent* SourceASC,
															 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
															 int32) const
{
	checkf(GameplayEffect, TEXT("No valid DamageEffectClass !"));

	if (bReplaceExistingInstance)
	{
		for (FActiveGameplayEffectHandle const& ActiveHandle :
			 TargetASC->GetActiveGameplayEffects().GetAllActiveEffectHandles())
		{
			FActiveGameplayEffect const* ActiveEffect = TargetASC->GetActiveGameplayEffect(ActiveHandle);
			if (ActiveEffect && ActiveEffect->Spec.Def && ActiveEffect->Spec.Def->GetClass() == GameplayEffect
				&& ActiveEffect->Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent() == SourceASC)
			{
				TargetASC->RemoveActiveGameplayEffect(ActiveHandle);
				break;
			}
		}
	}

	FGameplayEffectContextHandle SpecContextHandle = ContextHandle;
	if (Icon)
	{
		SpecContextHandle = ContextHandle.Duplicate();
		static_cast<FGeoGameplayEffectContext*>(SpecContextHandle.Get())->SetIcon(Icon);
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(GameplayEffect, AbilityLevel, SpecContextHandle);

	if (DataTag.IsValid())
	{
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DataTag,
																	  Magnitude.GetValueAtLevel(AbilityLevel));
	}

	float const DurationToSet = Duration.GetValueAtLevel(AbilityLevel);
	if (DurationToSet > 0.f)
	{
		FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Tags.Gameplay_DurationMagnitude,
																	  Duration.GetValueAtLevel(AbilityLevel));
	}

	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void FMagnitudeEffectData::UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32, FGameplayTag) const
{
	if (bSuppressGameplayCue)
	{
		EffectContext->SetSuppressGameplayCue(true);
	}
	if (bLimitGameplayCue || bIsPerSecond)
	{
		EffectContext->SetLimitGameplayCue(true);
	}
	if (bSuppressCombatStats)
	{
		EffectContext->SetSuppressCombatStats(true);
	}
}

FActiveGameplayEffectHandle FMagnitudeEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
															  UAbilitySystemComponent* SourceASC,
															  UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
															  int32) const
{
	TSubclassOf<UGameplayEffect> const EffectClass = GetEffectClass();
	if (!ensureMsgf(EffectClass, TEXT("%hs: UGameDataSettings has no instant effect for %s — add one."), __FUNCTION__,
					*GetMagnitudeTag().ToString()))
	{
		return FActiveGameplayEffectHandle();
	}

	if (!ensureMsgf(SourceASC && TargetASC, TEXT("%hs: no Source or Target ASC"), __FUNCTION__))
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, AbilityLevel, ContextHandle);

	float CalculatedAmount = Amount.GetValueAtLevel(AbilityLevel);
	if (bIsPerSecond)
	{

		CalculatedAmount *= SourceASC->GetWorld()->GetDeltaSeconds();
	}
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GetMagnitudeTag(), CalculatedAmount);

	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

FString FMagnitudeEffectData::GetDescriptionLine(FDescriptionFormat const& Format) const
{
	return FString::Printf(TEXT("%s: %s"), *GetTagLeafName(GetMagnitudeTag()), *FormatScalableRange(Amount, Format));
}

TSubclassOf<UGameplayEffect> FDamageEffectData::GetEffectClass() const
{
	return GetDefault<UGameDataSettings>()->DamageEffect.LoadSynchronous();
}

FGameplayTag FDamageEffectData::GetMagnitudeTag() const
{
	return FGeoGameplayTags::Get().Gameplay_Damage;
}

void FDamageEffectData::UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
											FGameplayTag AbilityTag) const
{
	FMagnitudeEffectData::UpdateContextHandle(EffectContext, AbilityLevel, AbilityTag);

	if (bDoNotRedirectSacrifice)
	{
		EffectContext->SetDoNotRedirectSacrifice(true);
	}

	// Basic-ability identity comes from the firing ability's own tags, not a per-effect flag. Many effect sources
	// (zones, drains) have no originating ability — guard the invalid tag so we never run the lookup or log a spurious
	// warning.
	if (!AbilityTag.IsValid())
	{
		return;
	}
	UGeoGameplayAbility const* AbilityCDO = UGeoAbilitySystemLibrary::GetAbilityCDO(AbilityTag);
	if (AbilityCDO && AbilityCDO->GetAssetTags().HasTag(FGeoGameplayTags::Get().Ability_Type_Basic))
	{
		EffectContext->SetIsFromBasicAbility(true);
	}
}

TSubclassOf<UGameplayEffect> FHealEffectData::GetEffectClass() const
{
	return GetDefault<UGameDataSettings>()->HealthEffect.LoadSynchronous();
}

FGameplayTag FHealEffectData::GetMagnitudeTag() const
{
	return FGeoGameplayTags::Get().Gameplay_Heal;
}

void FHealEffectData::UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32 AbilityLevel,
										  FGameplayTag AbilityTag) const
{
	FMagnitudeEffectData::UpdateContextHandle(EffectContext, AbilityLevel, AbilityTag);

	if (bSuppressHealProvided)
	{
		EffectContext->SetSuppressHealProvided(true);
	}
}

TSubclassOf<UGameplayEffect> FShieldEffectData::GetEffectClass() const
{
	return GetDefault<UGameDataSettings>()->ShieldEffect.LoadSynchronous();
}

FGameplayTag FShieldEffectData::GetMagnitudeTag() const
{
	return FGeoGameplayTags::Get().Gameplay_Shield;
}

FString FGameplayEffectData::GetDescriptionLine(FDescriptionFormat const& Format) const
{
	if (!GameplayEffect)
	{
		return FString();
	}

	FString const Name = DataTag.IsValid() ? GetTagLeafName(DataTag) : GameplayEffect->GetName();
	FString Line = FString::Printf(TEXT("%s: %s"), *Name, *FormatScalableRange(Magnitude, Format));
	if (Duration.GetValueAtLevel(Format.MinLevel()) > 0.f)
	{
		Line += FString::Printf(TEXT(" for %ss"), *FormatScalableRange(Duration, Format));
	}
	return Line;
}

void FContextDamageMultiplierEffectData::UpdateContextHandle(FGeoGameplayEffectContext* EffectContext,
															 int32 AbilityLevel, FGameplayTag) const
{
	ensureMsgf(Multiplier != 1.f,
			   TEXT("You've set Single Use Damage Multiplier but value is 1. So it's not useful, you douchebag !"));
	EffectContext->SetSingleUseDamageMultiplier(Multiplier.GetValueAtLevel(AbilityLevel));
}

FActiveGameplayEffectHandle FLethalEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
														   UAbilitySystemComponent* SourceASC,
														   UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
														   int32) const
{
	TSubclassOf<UGameplayEffect> const LethalEffectClass =
		GetDefault<UGameDataSettings>()->LethalEffect.LoadSynchronous();
	if (!ensureMsgf(LethalEffectClass, TEXT("Add a Lethal Effect in UGameDataSettings!")))
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(LethalEffectClass, AbilityLevel, ContextHandle);
	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

FString FContextDamageMultiplierEffectData::GetDescriptionLine(FDescriptionFormat const& Format) const
{
	FDescriptionFormat BonusPercentFormat = Format;
	BonusPercentFormat.ValueFormat = EValueFormat::BonusPercent;
	return FString::Printf(TEXT("%s more damage"), *FormatScalableRange(Multiplier, BonusPercentFormat));
}

FString FLethalEffectData::GetDescriptionLine(FDescriptionFormat const& /*Format*/) const
{
	return TEXT("Lethal");
}
