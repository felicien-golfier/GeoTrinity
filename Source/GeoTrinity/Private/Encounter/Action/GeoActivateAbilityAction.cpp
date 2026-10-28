// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoActivateAbilityAction.h"

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Abilities/Base/PatternAbilityParams.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/HiddenCasterCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "Encounter/GeoEncounterSubsystem.h"
#include "GeoTrinity/GeoTrinity.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
EDataValidationResult UGeoActivateAbilityAction::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);

	if (!AbilityClass)
	{
		Context.AddError(FText::FromString(FString::Printf(TEXT("%s: no AbilityClass"), *GetName())));
	}
	else if (GetDefault<UGeoGameplayAbility>(AbilityClass)->RequiresArena())
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s: %s needs a caster's arena, an encounter cannot fire it"), *GetName(), *AbilityClass->GetName())));
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void UGeoActivateAbilityAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	UGeoEncounterSubsystem* const Subsystem = GetWorld()->GetSubsystem<UGeoEncounterSubsystem>();
	FGameplayAbilitySpecHandle const Handle = Subsystem->GrantSharedAbility(AbilityClass);
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(Subsystem->GetSharedCaster());
	FGameplayAbilitySpec const* const Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
	if (!Spec || Spec->IsActive())
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: %s not fired on the shared caster: %s"), *GetName(),
			   *GetNameSafe(AbilityClass), Spec ? TEXT("already running") : TEXT("not granted"));
		Finish();
	}
	else
	{
		ActivatedASC = ASC;
		ActivatedHandle = Handle;
		if (bWaitForEnd)
		{
			AbilityEndedDelegateHandle =
				ASC->OnAbilityEnded.AddUObject(this, &UGeoActivateAbilityAction::OnAbilityEnded);
		}

		FPatternAbilityParams* const Params = new FPatternAbilityParams();
		Params->ArenaTag = ArenaTag;
		Params->PatternData = PatternData;
		FGameplayEventData ActivationData;
		ActivationData.TargetData.Add(Params);
		if (!ASC->InternalTryActivateAbility(Handle, FPredictionKey(), nullptr, nullptr, &ActivationData))
		{
			UE_LOG(LogGeoEncounter, Warning, TEXT("%s: %s refused to activate on the shared caster"), *GetName(),
				   *GetNameSafe(AbilityClass));
			StopWaiting();
			Finish();
		}
		else if (!bWaitForEnd)
		{
			Finish();
		}
	}
}

void UGeoActivateAbilityAction::OnCancel_Implementation()
{
	UAbilitySystemComponent* const ASC = ActivatedASC.Get();
	FGameplayAbilitySpecHandle const Handle = ActivatedHandle;
	bool const bWasWaiting = AbilityEndedDelegateHandle.IsValid();
	StopWaiting();
	if (ASC && (bWasWaiting || GetEncounter()->IsAborting()))
	{
		ASC->CancelAbilityHandle(Handle);
	}
}

void UGeoActivateAbilityAction::OnAbilityEnded(FAbilityEndedData const& EndedData)
{
	if (EndedData.AbilitySpecHandle == ActivatedHandle)
	{
		StopWaiting();
		Finish();
	}
}

void UGeoActivateAbilityAction::StopWaiting()
{
	if (UAbilitySystemComponent* const ASC = ActivatedASC.Get())
	{
		ASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
	}
	AbilityEndedDelegateHandle.Reset();
	ActivatedASC.Reset();
	ActivatedHandle = FGameplayAbilitySpecHandle();
}
