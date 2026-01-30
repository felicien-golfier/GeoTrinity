// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------------------------------------------------
UExecCalc_Damage::UExecCalc_Damage()
{
	// Add releveant attributes to capture (once we have them). For example: crit chance, armor, etc
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
	float damage = specGE.GetSetByCallerMagnitude(tags.Gameplay_Damage, false, 0.f);

#pragma region Radial damage
	if (UGeoAbilitySystemLibrary::GetIsRadialDamage(contextHandle))
	{
		UGameplayStatics::ApplyRadialDamageWithFalloff(
			pTargetAvatar, damage, 0.f, UGeoAbilitySystemLibrary::GetRadialDamageOrigin(contextHandle),
			UGeoAbilitySystemLibrary::GetRadialDamageInnerRadius(contextHandle),
			UGeoAbilitySystemLibrary::GetRadialDamageOuterRadius(contextHandle), 1.f, UDamageType::StaticClass(),
			TArray<AActor*>(), pSourceAvatar, nullptr);
	}
#pragma endregion

	/*** OUTPUT ***/
	FGameplayModifierEvaluatedData const evaluatedData{UGeoAttributeSetBase::GetIncomingDamageAttribute(),
													   EGameplayModOp::Additive, damage};
	OutExecutionOutput.AddOutputModifier(std::move(evaluatedData));
}
