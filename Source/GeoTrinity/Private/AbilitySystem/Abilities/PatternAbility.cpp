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
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	ASC->PatternStartMulticast(Payload, PatternToLaunch);
	UPattern* PatternInstance;
	ensureMsgf(ASC->FindPatternByClass(PatternToLaunch, PatternInstance),
			   TEXT("Pattern Instance doesn't exist when launching PatternAbility !"));
	PatternInstance->OnPatternEnd.AddUniqueDynamic(this, &UPatternAbility::OnPatternEnd);
}

void UPatternAbility::OnPatternEnd()
{
	UGeoAbilitySystemComponent* ASC = GetGeoAbilitySystemComponentFromActorInfo();
	UPattern* PatternInstance;
	ensureMsgf(ASC->FindPatternByClass(PatternToLaunch, PatternInstance),
			   TEXT("Pattern Instance doesn't exist when launching PatternAbility !"));
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), false, false);
	PatternInstance->OnPatternEnd.RemoveDynamic(this, &UPatternAbility::OnPatternEnd);
}
