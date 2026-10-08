// Copyright 2024 GeoTrinity. All Rights Reserved.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"

#include "AbilitySystem/AttributeSet/CharacterAttributeSet.h"
#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "AbilitySystem/Lib/GeoGameplayTags.h"
#include "AbilitySystem/Types/GeoAscTypes.h"
#include "AbilitySystemComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
void UExecCalc_Damage::Execute_Implementation(FGameplayEffectCustomExecutionParameters const& ExecutionParams,
											  FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	/** GET SOURCE DATA **/
	FGameplayEffectSpec const& EffectSpec = ExecutionParams.GetOwningSpec();
	FGameplayEffectContextHandle ContextHandle = EffectSpec.GetContext();
	UAbilitySystemComponent const* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent const* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	AActor* TargetAvatar = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
	FGeoGameplayTags const& Tags = FGeoGameplayTags::Get();

	float Damage = EffectSpec.GetSetByCallerMagnitude(Tags.Gameplay_Damage, false, 0.f);

	FGeoGameplayEffectContext const* GeoContext = static_cast<FGeoGameplayEffectContext const*>(ContextHandle.Get());
	if (GeoContext)
	{
		Damage *= GeoContext->GetSingleUseDamageMultiplier();
		if (GeoASLib::ShouldSuppressGameplayCue(*GeoContext, TargetAvatar, /*bIsHeal*/ false))
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

	if (!GeoContext || !GeoContext->ShouldSkipStatModifiers())
	{
		float const DamageReduction =
			GeoASLib::GetStatValue(TargetASC, UCharacterAttributeSet::GetDamageReductionAttribute(), 0.f);
		Damage *= GeoASLib::GetStatValue(SourceASC, UCharacterAttributeSet::GetDamageMultiplierAttribute(), 1.f)
			* (1.f - FMath::Clamp(DamageReduction, 0.f, 1.f)) * GeoASLib::RollCritMultiplier(SourceASC);
	}

	/*** OUTPUT ***/
	FGameplayModifierEvaluatedData const evaluatedData{UGeoAttributeSetBase::GetIncomingDamageAttribute(),
													   EGameplayModOp::Additive, Damage};
	OutExecutionOutput.AddOutputModifier(std::move(evaluatedData));
}
