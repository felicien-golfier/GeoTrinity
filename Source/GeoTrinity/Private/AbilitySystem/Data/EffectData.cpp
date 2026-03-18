#include "AbilitySystem/Data/EffectData.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"

void FEffectData::UpdateContextHandle(FGeoGameplayEffectContext*) const
{
	ensureMsgf(false, TEXT("UpdateContextHandle not implemented for this effect %s"), *StaticStruct()->GetName());
}
void FEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
							  UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const
{
	ensureMsgf(false, TEXT("ApplyEffect not implemented for this effect %s"), *StaticStruct()->GetName());
}

void FDamageEffectData::UpdateContextHandle(FGeoGameplayEffectContext*) const
{
	checkf(DamageEffectClass, TEXT("No valid DamageEffectClass !"));
}

void FDamageEffectData::ApplyEffect(FGameplayEffectContextHandle const& ContextHandle,
									UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC,
									int32 AbilityLevel, int32) const
{
	FGameplayEffectSpecHandle specHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, AbilityLevel, ContextHandle);

	FGeoGameplayTags const& tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(specHandle, tags.Gameplay_Damage,
																  DamageAmount.GetValueAtLevel(AbilityLevel));

	TargetASC->ApplyGameplayEffectSpecToSelf(*specHandle.Data);
}

void FSingleUseDamageMultiplierEffectData::UpdateContextHandle(FGeoGameplayEffectContext* EffectContext) const
{
	checkf(Multiplier != 1.f,
		   TEXT("You've set Single Use Damage Multiplier but value is 1. So it's not useful, you douchebag !"));
	EffectContext->SetSingleUseDamageMultiplier(Multiplier);
}

void FStatusEffectData::UpdateContextHandle(FGeoGameplayEffectContext* GeoGameplayEffectContext) const
{
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
