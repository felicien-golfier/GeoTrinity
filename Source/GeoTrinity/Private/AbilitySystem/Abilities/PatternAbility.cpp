// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/PatternAbility.h"

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"

void UPatternAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	checkf(PatternToLaunch, TEXT("Please fill the PatternToLaunch in Blueprint"));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	AActor* Owner = GetOwningActorFromActorInfo();
	const FAbilityPayload& Payload = CreateAbilityPayload(Owner->GetTransform(), Owner, Owner);
	GetGeoAbilitySystemComponentFromActorInfo()->PatternStartMulticast(Payload);

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}