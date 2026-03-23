#include "AbilitySystem/Data/EffectData.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"

void FEffectData::UpdateContextHandle(FGeoGameplayEffectContext*, int32) const
{
	// By default does nothing. Override for your needs
}
void FEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
							  UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const
{
	// By default does nothing. Override for your needs
}

void FGameplayEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
									  UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC,
									  int32 AbilityLevel, int32) const
{
	checkf(GameplayEffect, TEXT("No valid DamageEffectClass !"));

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(GameplayEffect, AbilityLevel, ContextHandle);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DataTag,
																  Magnitude.GetValueAtLevel(AbilityLevel));
	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Tags.Gameplay_DurationMagnitude,
																  Duration.GetValueAtLevel(AbilityLevel));

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void FDamageEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
									UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC,
									int32 AbilityLevel, int32) const
{
	checkf(DamageEffectClass, TEXT("No valid DamageEffectClass !"));
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, AbilityLevel, ContextHandle);

	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Tags.Gameplay_Damage,
																  DamageAmount.GetValueAtLevel(AbilityLevel));

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void FHealEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
								  UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC,
								  int32 AbilityLevel, int32) const
{
	checkf(HealEffectClass, TEXT("No valid HealEffectClass !"));
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(HealEffectClass, AbilityLevel, ContextHandle);

	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Tags.Gameplay_Heal,
																  HealAmount.GetValueAtLevel(AbilityLevel));

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void FContextDamageMultiplierEffectData::UpdateContextHandle(FGeoGameplayEffectContext* EffectContext,
															 int32 AbilityLevel) const
{
	checkf(Multiplier != 1.f,
		   TEXT("You've set Single Use Damage Multiplier but value is 1. So it's not useful, you douchebag !"));
	EffectContext->SetSingleUseDamageMultiplier(Multiplier.GetValueAtLevel(AbilityLevel));
}

void FStatusEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
									UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC,
									int32 AbilityLevel, int32 Seed) const
{
	if (static_cast<float>(Seed) / MAX_int32 * 100 <= StatusChance)
	{
		UGeoAbilitySystemLibrary::ApplyStatusToTarget(TargetASC, SourceASC, StatusTag, AbilityLevel);
	}
}
