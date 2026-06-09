// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Base/PatternAbility.h"

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "Tool/UGeoGameplayLibrary.h"

void UPatternAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo,
									  FGameplayEventData const* TriggerEventData)
{
	ensureMsgf(PatternToLaunch, TEXT("Please fill the PatternToLaunch in Blueprint"));
	ensureMsgf(GeoLib::IsServer(GetWorld()), TEXT("PatternAbility are made for Server initiated abilities only."));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	StoredPayload = CreateAbilityPayload();
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	ASC->PatternStartMulticast(StoredPayload, PatternToLaunch);
	UPattern* PatternInstance = nullptr;
	if (!ASC->FindPatternByClass(PatternToLaunch, PatternInstance))
	{
		ensureMsgf(false, TEXT("Pattern Instance doesn't exist when launching PatternAbility !"));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	PatternInstance->OnPatternEnd.AddUniqueDynamic(this, &UPatternAbility::OnPatternEnd);
}

void UPatternAbility::OnPatternEnd()
{
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	UPattern* PatternInstance = nullptr;
	if (!ASC->FindPatternByClass(PatternToLaunch, PatternInstance))
	{
		ensureMsgf(false, TEXT("Pattern Instance doesn't exist at the end of the pattern on server  !"));
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, true);
		return;
	}
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
	PatternInstance->OnPatternEnd.RemoveDynamic(this, &UPatternAbility::OnPatternEnd);
}
void UPatternAbility::EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
								 bool bWasCancelled)
{
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	UPattern* PatternInstance = nullptr;
	if (!ASC->FindPatternByClass(PatternToLaunch, PatternInstance))
	{
		ensureMsgf(false, TEXT("Pattern Instance doesn't exist at ability end !"));
	}
	else
	{
		PatternInstance->EndPattern(true);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
