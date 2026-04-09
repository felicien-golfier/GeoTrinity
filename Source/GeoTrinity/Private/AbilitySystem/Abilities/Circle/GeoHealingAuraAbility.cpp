// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoHealingAuraAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealingAuraAbility::OnAvatarSet(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealingAuraAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,
											 FGameplayAbilityActorInfo const* ActorInfo,
											 FGameplayAbilityActivationInfo ActivationInfo,
											 FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	if (!HealPerSecond.IsValid())
	{
		ensureMsgf(HealPerSecond.IsValid(), TEXT("Fill your data dumb ass"));
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealingAuraAbility::Tick(float const DeltaTime)
{
	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		ensureMsgf(IsValid(Character), TEXT("UGeoHealingAuraAbility: invalid Character on activation"));
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	IGenericTeamAgentInterface const* OwnerTeamAgent = Cast<IGenericTeamAgentInterface>(Character);
	if (!SourceASC || !OwnerTeamAgent)
	{
		ensureMsgf(SourceASC && OwnerTeamAgent,
				   TEXT("UGeoHealingAuraAbility: invalid ASC or OwnerTeamAgent on activation"));
		return;
	}

	TArray<AActor*> OverlappingActors;
	Character->GetCapsuleComponent()->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor == Character || OwnerTeamAgent->GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile)
		{
			continue;
		}

		UGeoAbilitySystemComponent* TargetASC = UGeoAbilitySystemLibrary::GetGeoAscFromActor(Actor);
		if (!TargetASC)
		{
			continue;
		}

		if (TargetASC->GetNumericAttribute(UGeoAttributeSetBase::GetHealthAttribute())
			>= TargetASC->GetNumericAttribute(UGeoAttributeSetBase::GetMaxHealthAttribute()))
		{
			continue; // Do not heal, neither count in AlliesHealed full life mates.
		}

		FHealEffectData HealEffect = FHealEffectData();
		HealEffect.HealAmount = HealPerSecond.GetValueAtLevel(GetAbilityLevel()) * DeltaTime;
		UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffect, SourceASC, TargetASC, GetAbilityLevel(),
														StoredPayload.Seed);
	}
}
