// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoHealingAuraAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Characters/Component/GeoGameFeelComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

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
		if (!Actor->CanBeDamaged() || Actor == Character
			|| OwnerTeamAgent->GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile)
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

		UGeoGameFeelComponent* GameFeelComponent = Actor->FindComponentByClass<UGeoGameFeelComponent>();
		if (!ensureMsgf(GameFeelComponent, TEXT("UGeoHealingAuraAbility: avatar has no GeoGameFeelComponent")))
		{
			return;
		}
		bool const bSuppressHealCue = !GameFeelComponent->IsHealCueAvailable();

		FHealEffectData HealEffect;
		HealEffect.HealAmount = HealPerSecond.GetValueAtLevel(GetAbilityLevel()) * DeltaTime;
		HealEffect.bSuppressGameplayCue = bSuppressHealCue;
		UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffect, SourceASC, TargetASC, GetAbilityLevel(),
														StoredPayload.Seed);
	}
}
