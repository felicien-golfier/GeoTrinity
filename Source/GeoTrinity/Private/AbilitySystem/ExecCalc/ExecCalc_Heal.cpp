// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/ExecCalc/ExecCalc_Heal.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"

// ---------------------------------------------------------------------------------------------------------------------
UExecCalc_Heal::UExecCalc_Heal()
{
	HealMultiplierCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UCharacterAttributeSet::GetHealMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
	RelevantAttributesToCapture.Add(HealMultiplierCaptureDef);
}

// ---------------------------------------------------------------------------------------------------------------------
void UExecCalc_Heal::Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
											FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	FGameplayEffectSpec const& specGE = ExecutionParams.GetOwningSpec();
	FGeoGameplayTags const& tags = FGeoGameplayTags::Get();

	float HealAmount = specGE.GetSetByCallerMagnitude(tags.Gameplay_Heal, false, 0.f);

	FAggregatorEvaluateParameters EvaluationParams;
	EvaluationParams.SourceTags = specGE.CapturedSourceTags.GetAggregatedTags();
	EvaluationParams.TargetTags = specGE.CapturedTargetTags.GetAggregatedTags();

	float HealMultiplier = 1.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HealMultiplierCaptureDef, EvaluationParams,
															   HealMultiplier);
	HealAmount *= HealMultiplier;

	FGameplayModifierEvaluatedData const evaluatedData{UGeoAttributeSetBase::GetIncomingHealAttribute(),
													   EGameplayModOp::Additive, HealAmount};
	OutExecutionOutput.AddOutputModifier(std::move(evaluatedData));
}
