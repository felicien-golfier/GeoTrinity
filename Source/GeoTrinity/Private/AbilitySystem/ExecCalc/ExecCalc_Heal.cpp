// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/ExecCalc/ExecCalc_Heal.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
UExecCalc_Heal::UExecCalc_Heal()
{
	// TODO: Here the heal multiplicator is on the target. It's not the intend way. it is a received heal multiplicator
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

	FGeoGameplayEffectContext const* GeoContext =
		static_cast<FGeoGameplayEffectContext const*>(specGE.GetContext().Get());
	if (GeoContext)
	{
		bool bSuppressGameplayCue = GeoContext->IsSuppressGameplayCue();
		UAbilitySystemComponent const* pTargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
		AActor* pTargetAvatar = pTargetASC ? pTargetASC->GetAvatarActor() : nullptr;
		if (!bSuppressGameplayCue && GeoContext->IsLimitGameplayCue() && IsValid(pTargetAvatar))
		{
			UGeoGameFeelComponent* GameFeelComponent = pTargetAvatar->FindComponentByClass<UGeoGameFeelComponent>();
			if (ensureMsgf(GameFeelComponent,
						   TEXT("UExecCalc_Heal: bLimitGameplayCue set but target %s has no GeoGameFeelComponent"),
						   *pTargetAvatar->GetName()))
			{
				bSuppressGameplayCue = !GameFeelComponent->IsHealCueAvailable();
			}
		}
		if (bSuppressGameplayCue)
		{
			OutExecutionOutput.MarkGameplayCuesHandledManually();
		}
	}

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
