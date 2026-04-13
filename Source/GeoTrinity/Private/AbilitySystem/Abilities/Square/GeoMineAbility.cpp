// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Square/GeoMineAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
bool UGeoMineAbility::CheckCost(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								FGameplayTagContainer* OptionalRelevantTags) const
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ensureMsgf(ASC, TEXT("UGeoMineAbility::CheckCost: no ASC on ActorInfo")))
	{
		return false;
	}

	if (ASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute()) <= 1.f)
	{
		return false;
	}

	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoMineAbility::OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
											   FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	Params.Value = SourceASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());

	UGeoAbilitySystemLibrary::ApplyEffectFromEffectData(GetEffectDataArray(), SourceASC, SourceASC,
														StoredPayload.AbilityLevel, StoredPayload.Seed);
	Params.Value = Params.Value - SourceASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute());

	Super::OnFireTargetDataReceived(DataHandle, ApplicationTag);
}
