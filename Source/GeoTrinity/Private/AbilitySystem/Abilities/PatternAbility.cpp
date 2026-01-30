// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/PatternAbility.h"

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"

void UPatternAbility::ActivateAbility(FGameplayAbilitySpecHandle const Handle,
									  FGameplayAbilityActorInfo const* ActorInfo,
									  FGameplayAbilityActivationInfo const ActivationInfo,
									  FGameplayEventData const* TriggerEventData)
{
	ensureMsgf(PatternToLaunch, TEXT("Please fill the PatternToLaunch in Blueprint"));
	ensureMsgf(HasAuthority(&ActivationInfo), TEXT("PatternAbility are made for Server initiated abilities only."));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	AActor* Owner = GetOwningActorFromActorInfo();
	FAbilityPayload const& Payload = CreateAbilityPayload(Owner, Owner, Owner->GetTransform());
	GetGeoAbilitySystemComponentFromActorInfo()->PatternStartMulticast(Payload, PatternToLaunch);

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
