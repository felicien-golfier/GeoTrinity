#include "AbilitySystem/Data/EffectData.h"

#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"

void UEffectDataAsset::UpdateContextHandle(FGeoGameplayEffectContext*) const
{
	ensureMsgf(false, TEXT("UpdateContextHandle not implemented for this effect %s"), *GetClass()->GetName());
}
void UEffectDataAsset::ApplyEffect(const FGameplayEffectContextHandle& ContextHandle,
	UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const
{
	ensureMsgf(false, TEXT("UpdateContextHandle not implemented for this effect %s"), *GetClass()->GetName());
}

void UDamageEffectData::UpdateContextHandle(FGeoGameplayEffectContext*) const
{
	checkf(DamageEffectClass, TEXT("No valid DamageEffectClass !"));
}

void UDamageEffectData::ApplyEffect(const FGameplayEffectContextHandle& ContextHandle,
	UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32) const
{
	FGameplayEffectSpecHandle specHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, AbilityLevel, ContextHandle);

	/** Type of damage **/
	const FGeoGameplayTags& tags = FGeoGameplayTags::Get();
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(specHandle, tags.Gameplay_Damage,
		DamageAmount.GetValueAtLevel(AbilityLevel));

	/** APPLY EFFECT **/
	TargetASC->ApplyGameplayEffectSpecToSelf(*specHandle.Data);
}

void UStatusEffectData::UpdateContextHandle(FGeoGameplayEffectContext* GeoGameplayEffectContext) const
{
}

void UStatusEffectData::ApplyEffect(const FGameplayEffectContextHandle& ContextHandle,
	UGeoAbilitySystemComponent* SourceASC, UGeoAbilitySystemComponent* TargetASC, int32 AbilityLevel, int32 Seed) const
{
	if (static_cast<float>(Seed) / MAX_int32 * 100 <= StatusChance)
	{
		UGeoAbilitySystemLibrary::ApplyStatusToTarget(TargetASC, SourceASC, StatusTag, AbilityLevel);
	}
}