#include "AbilitySystem/Data/EffectData.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Settings/GameDataSettings.h"

void FEffectData::UpdateContextHandle(FGeoGameplayEffectContext*, int32) const
{
	// By default does nothing. Override for your needs
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

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(GameplayEffect, AbilityLevel, ContextHandle);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DataTag,
																  Magnitude.GetValueAtLevel(AbilityLevel));
	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Tags.Gameplay_DurationMagnitude,
																  Duration.GetValueAtLevel(AbilityLevel));

	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

FActiveGameplayEffectHandle FDamageEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
														   UAbilitySystemComponent* SourceASC,
														   UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
														   int32) const
{
	TSubclassOf<UGameplayEffect> const DamageEffectClass =
		GetDefault<UGameDataSettings>()->DamageEffect.LoadSynchronous();
	if (!DamageEffectClass)
	{
		ensureMsgf(DamageEffectClass, TEXT("Add an instant Damage Effect using ExecCalc_Damage in UGameDataSettings!"));
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, AbilityLevel, ContextHandle);

	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Tags.Gameplay_Damage,
																  DamageAmount.GetValueAtLevel(AbilityLevel));

	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void FHealEffectData::UpdateContextHandle(FGeoGameplayEffectContext* EffectContext, int32) const
{
	if (bSuppressHealProvided)
	{
		EffectContext->SetSuppressHealProvided(true);
	}
}

FActiveGameplayEffectHandle FHealEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
														 UAbilitySystemComponent* SourceASC,
														 UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
														 int32) const
{
	TSubclassOf<UGameplayEffect> const HealEffectClass =
		GetDefault<UGameDataSettings>()->HealthEffect.LoadSynchronous();
	if (!HealEffectClass)
	{
		ensureMsgf(HealEffectClass, TEXT("Add an instant Heal Effect using ExecCalc_Heal in UGameDataSettings!"));
		return FActiveGameplayEffectHandle();
	}
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(HealEffectClass, AbilityLevel, ContextHandle);

	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Tags.Gameplay_Heal,
																  HealAmount.GetValueAtLevel(AbilityLevel));

	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

FActiveGameplayEffectHandle FShieldEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
														   UAbilitySystemComponent* SourceASC,
														   UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
														   int32) const
{
	TSubclassOf<UGameplayEffect> const ShieldEffectClass =
		GetDefault<UGameDataSettings>()->ShieldEffect.LoadSynchronous();
	if (!ensureMsgf(ShieldEffectClass, TEXT("Add a Shield Effect in UGameDataSettings!")))
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(ShieldEffectClass, AbilityLevel, ContextHandle);

	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Tags.Gameplay_Shield,
																  ShieldAmount.GetValueAtLevel(AbilityLevel));

	return TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void FContextDamageMultiplierEffectData::UpdateContextHandle(FGeoGameplayEffectContext* EffectContext,
															 int32 AbilityLevel) const
{
	checkf(Multiplier != 1.f,
		   TEXT("You've set Single Use Damage Multiplier but value is 1. So it's not useful, you douchebag !"));
	EffectContext->SetSingleUseDamageMultiplier(Multiplier.GetValueAtLevel(AbilityLevel));
}

FActiveGameplayEffectHandle FStatusEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
														   UAbilitySystemComponent* SourceASC,
														   UAbilitySystemComponent* TargetASC, int32 AbilityLevel,
														   int32 Seed) const
{
	// Seed is a deterministic pseudo-random value replicated to all machines; mapping it to [0,100] gives
	// a consistent proc roll — all machines agree on whether the status was applied without a server RPC.
	if (static_cast<float>(Seed) / MAX_int32 * 100 <= StatusChance)
	{
		FGameplayEffectSpecHandle SpecHandle;
		return UGeoAbilitySystemLibrary::ApplyStatusToTarget(TargetASC, SourceASC, StatusTag, AbilityLevel, SpecHandle);
	}

	return FActiveGameplayEffectHandle();
}
