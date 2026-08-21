// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/ExecCalc/ExecCalc_Heal.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
UExecCalc_Heal::UExecCalc_Heal()
{
	AppliedHealBoostCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UCharacterAttributeSet::GetAppliedHealBoostAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	RelevantAttributesToCapture.Add(AppliedHealBoostCaptureDef);

	ReceivedHealBoostCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UCharacterAttributeSet::GetReceivedHealBoostAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
	RelevantAttributesToCapture.Add(ReceivedHealBoostCaptureDef);
}

// ---------------------------------------------------------------------------------------------------------------------
void UExecCalc_Heal::Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
											FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	FGameplayEffectSpec const& EffectSpec = ExecutionParams.GetOwningSpec();
	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();

	FGeoGameplayEffectContext const* GeoContext =
		static_cast<FGeoGameplayEffectContext const*>(EffectSpec.GetContext().Get());
	if (GeoContext)
	{
		UAbilitySystemComponent const* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
		AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
		if (GeoASLib::ShouldSuppressGameplayCue(*GeoContext, TargetAvatar, /*bIsHeal*/ true))
		{
			OutExecutionOutput.MarkGameplayCuesHandledManually();
		}
	}

	float HealAmount = EffectSpec.GetSetByCallerMagnitude(Tags.Gameplay_Heal, false, 0.f);

	FAggregatorEvaluateParameters EvaluationParams;
	EvaluationParams.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParams.TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	float AppliedHealBoost = 1.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(AppliedHealBoostCaptureDef, EvaluationParams,
															   AppliedHealBoost);

	float ReceivedHealBoost = 1.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ReceivedHealBoostCaptureDef, EvaluationParams,
															   ReceivedHealBoost);

	HealAmount *= AppliedHealBoost * ReceivedHealBoost;

	FGameplayModifierEvaluatedData const evaluatedData{UGeoAttributeSetBase::GetIncomingHealAttribute(),
													   EGameplayModOp::Additive, HealAmount};
	OutExecutionOutput.AddOutputModifier(std::move(evaluatedData));
}
