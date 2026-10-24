// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "Encounter/Action/GeoActivateAbilityAction.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Actor/Arena/GeoArena.h"
#include "Characters/EnemyCharacter.h"
#include "Encounter/GeoEncounter.h"
#include "GeoTrinity/GeoTrinity.h"

void UGeoActivateAbilityAction::Execute_Implementation(FGeoEncounterPayload const& /*Payload*/)
{
	UGeoAbilitySystemComponent* const ASC = GeoASLib::GetGeoAscFromActor(IsValid(Arena) ? Arena->GetBoss() : nullptr);
	TArray<FGameplayAbilitySpec*> Specs;
	if (ASC)
	{
		ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(AbilityTag), Specs, false);
	}

	if (Specs.IsEmpty())
	{
		UE_LOG(LogGeoEncounter, Warning, TEXT("%s: no ability tagged %s on the boss of %s"), *GetName(),
			   *AbilityTag.ToString(), *GetNameSafe(Arena));
		Finish();
	}
	else
	{
		FGameplayAbilitySpecHandle const Handle = Specs[0]->Handle;
		ActivatedASC = ASC;
		ActivatedHandle = Handle;
		if (bWaitForEnd)
		{
			AbilityEndedDelegateHandle =
				ASC->OnAbilityEnded.AddUObject(this, &UGeoActivateAbilityAction::OnAbilityEnded);
		}

		FGameplayEventData ActivationData;
		ActivationData.OptionalObject = this;
		if (!ASC->InternalTryActivateAbility(Handle, FPredictionKey(), nullptr, nullptr, &ActivationData))
		{
			UE_LOG(LogGeoEncounter, Warning, TEXT("%s: %s refused to activate on the boss of %s"), *GetName(),
				   *AbilityTag.ToString(), *GetNameSafe(Arena));
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
