// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemComponent.h"
#include "Characters/Component/GeoGameFeelComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
UExecCalc_Damage::UExecCalc_Damage()
{
	DamageMultiplierCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UCharacterAttributeSet::GetDamageMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	RelevantAttributesToCapture.Add(DamageMultiplierCaptureDef);

	DamageReductionCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UCharacterAttributeSet::GetDamageReductionAttribute(), EGameplayEffectAttributeCaptureSource::Target, true);
	RelevantAttributesToCapture.Add(DamageReductionCaptureDef);
}

// ---------------------------------------------------------------------------------------------------------------------
void UExecCalc_Damage::Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
											  FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	/** GET SOURCE DATA **/
	FGameplayEffectSpec const& specGE = ExecutionParams.GetOwningSpec();
	FGameplayEffectContextHandle contextHandle = specGE.GetContext();
	UAbilitySystemComponent const* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent const* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();

	float Damage = specGE.GetSetByCallerMagnitude(Tags.Gameplay_Damage, false, 0.f);

	FGeoGameplayEffectContext const* GeoContext = static_cast<FGeoGameplayEffectContext const*>(contextHandle.Get());
	if (GeoContext)
	{
		Damage *= GeoContext->GetSingleUseDamageMultiplier();
		bool bSuppressGameplayCue = GeoContext->IsSuppressGameplayCue();
		if (!bSuppressGameplayCue && GeoContext->IsLimitGameplayCue() && IsValid(TargetAvatar))
		{
			UGeoGameFeelComponent* GameFeelComponent = TargetAvatar->FindComponentByClass<UGeoGameFeelComponent>();
			if (ensureMsgf(GameFeelComponent,
						   TEXT("UExecCalc_Damage: bLimitGameplayCue set but target %s has no GeoGameFeelComponent"),
						   *TargetAvatar->GetName()))
			{
				bSuppressGameplayCue = !GameFeelComponent->IsDamageCueAvailable();
			}
		}

		if (bSuppressGameplayCue)
		{
			OutExecutionOutput.MarkGameplayCuesHandledManually();
		}

		if (GeoContext->IsFromBasicAbility() && IsValid(TargetAvatar))
		{
			if (UGeoAbilitySystemComponent* SourceGeoASC =
					Cast<UGeoAbilitySystemComponent>(const_cast<UAbilitySystemComponent*>(SourceASC)))
			{
				SourceGeoASC->SetLastBasicAbilityTarget(TargetAvatar);
			}
		}
	}

	FAggregatorEvaluateParameters EvaluationParams;
	EvaluationParams.SourceTags = specGE.CapturedSourceTags.GetAggregatedTags();
	EvaluationParams.TargetTags = specGE.CapturedTargetTags.GetAggregatedTags();

	float DamageMultiplier = 1.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageMultiplierCaptureDef, EvaluationParams,
															   DamageMultiplier);
	Damage *= DamageMultiplier;

	float DamageReduction = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageReductionCaptureDef, EvaluationParams,
															   DamageReduction);
	Damage *= 1.f - FMath::Clamp(DamageReduction, 0.f, 1.f);

	/*** OUTPUT ***/
	FGameplayModifierEvaluatedData const evaluatedData{UGeoAttributeSetBase::GetIncomingDamageAttribute(),
													   EGameplayModOp::Additive, Damage};
	OutExecutionOutput.AddOutputModifier(std::move(evaluatedData));
}
