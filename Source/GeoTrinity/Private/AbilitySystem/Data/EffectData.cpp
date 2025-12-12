#include "AbilitySystem/Data/EffectData.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"

void FEffectData::UpdateContextHandle(FGeoGameplayEffectContext*)
{
	ensureMsgf(false, TEXT("UpdateContextHandle not implemented for this effect %s"), *StaticStruct()->GetName());
}
void FEffectData::ApplyEffect(const FGameplayEffectContextHandle& ContextHandle, UGeoAbilitySystemComponent* SourceASC,
	UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed)
{
	ensureMsgf(false, TEXT("UpdateContextHandle not implemented for this effect %s"), *StaticStruct()->GetName());
}

void FDamageEffectData::UpdateContextHandle(FGeoGameplayEffectContext*)
{
	checkf(DamageEffectClass, TEXT("No valid DamageEffectClass !"));
}

void FDamageEffectData::ApplyEffect(const FGameplayEffectContextHandle& ContextHandle,
	UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32)
{
	FGameplayEffectSpecHandle specHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, AbilityLevel, ContextHandle);

	/** Type of damage **/
	const FGeoGameplayTags& tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(specHandle, tags.Gameplay_Damage,
		DamageAmount.GetValueAtLevel(AbilityLevel));

	/** APPLY EFFECT **/
	TargetASC->ApplyGameplayEffectSpecToSelf(*specHandle.Data);
}

void FStatusEffectData::UpdateContextHandle(FGeoGameplayEffectContext* GeoGameplayEffectContext)
{
}
void FStatusEffectData::ApplyEffect(const FGameplayEffectContextHandle& ContextHandle,
	UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed)
{
	if (static_cast<float>(Seed) / MAX_int32 * 100 <= StatusChance)
	{
		UGeoAbilitySystemLibrary::ApplyStatusToTarget(TargetASC, SourceASC, StatusTag, AbilityLevel);
	}
}