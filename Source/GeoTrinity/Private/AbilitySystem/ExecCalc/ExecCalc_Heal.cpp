// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/ExecCalc/ExecCalc_Heal.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
void UExecCalc_Heal::Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
											FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	FGameplayEffectSpec const& EffectSpec = ExecutionParams.GetOwningSpec();
	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();
	UAbilitySystemComponent const* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent const* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	FGeoGameplayEffectContext const* GeoContext =
		static_cast<FGeoGameplayEffectContext const*>(EffectSpec.GetContext().Get());
	if (GeoContext)
	{
		AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
		if (GeoASLib::ShouldSuppressGameplayCue(*GeoContext, TargetAvatar, /*bIsHeal*/ true))
		{
			OutExecutionOutput.MarkGameplayCuesHandledManually();
		}
	}

	float HealAmount = EffectSpec.GetSetByCallerMagnitude(Tags.Gameplay_Heal, false, 0.f);

	HealAmount *= GeoASLib::GetStatValue(SourceASC, UCharacterAttributeSet::GetAppliedHealBoostAttribute(), 1.f)
		* GeoASLib::GetStatValue(TargetASC, UCharacterAttributeSet::GetReceivedHealBoostAttribute(), 1.f)
		* GeoASLib::RollCritMultiplier(SourceASC);

	FGameplayModifierEvaluatedData const evaluatedData{UGeoAttributeSetBase::GetIncomingHealAttribute(),
													   EGameplayModOp::Additive, HealAmount};
	OutExecutionOutput.AddOutputModifier(std::move(evaluatedData));
}
