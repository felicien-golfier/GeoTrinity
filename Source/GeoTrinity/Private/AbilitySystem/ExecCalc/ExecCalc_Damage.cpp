// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/GeoAscTypes.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

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
	UAbilitySystemComponent const* pSourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent const* pTargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	AActor* pSourceAvatar = pSourceASC ? pSourceASC->GetAvatarActor() : nullptr;
	AActor* pTargetAvatar = pTargetASC ? pTargetASC->GetAvatarActor() : nullptr;
	FGeoGameplayTags const& tags = FGeoGameplayTags::Get();

	/** COMPUTE EVERYTHING TODO **/
	float Damage = specGE.GetSetByCallerMagnitude(tags.Gameplay_Damage, false, 0.f);

	FGeoGameplayEffectContext const* GeoContext = static_cast<FGeoGameplayEffectContext const*>(contextHandle.Get());
	if (GeoContext)
	{
		Damage *= GeoContext->GetSingleUseDamageMultiplier();
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

#pragma region Radial damage
	if (UGeoAbilitySystemLibrary::GetIsRadialDamage(contextHandle))
	{
		UGameplayStatics::ApplyRadialDamageWithFalloff(
			pTargetAvatar, Damage, 0.f, UGeoAbilitySystemLibrary::GetRadialDamageOrigin(contextHandle),
			UGeoAbilitySystemLibrary::GetRadialDamageInnerRadius(contextHandle),
			UGeoAbilitySystemLibrary::GetRadialDamageOuterRadius(contextHandle), 1.f, UDamageType::StaticClass(),
			TArray<AActor*>(), pSourceAvatar, nullptr);
	}
#pragma endregion

	/*** OUTPUT ***/
	FGameplayModifierEvaluatedData const evaluatedData{UGeoAttributeSetBase::GetIncomingDamageAttribute(),
													   EGameplayModOp::Additive, Damage};
	OutExecutionOutput.AddOutputModifier(std::move(evaluatedData));
}
