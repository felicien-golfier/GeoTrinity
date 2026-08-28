// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoHealingAuraAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Tool/UGeoGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------
UGeoHealingAuraAbility::UGeoHealingAuraAbility()
	// Only game-thread constructions may register as tickable: async-loaded Blueprint CDOs are built on the loading
	// thread and must not register (registration is game-thread-only; the CDO never ticks anyway).
	: FTickableGameObject(IsInGameThread() ? ETickableTickType::Conditional : ETickableTickType::Never)
{
	// Passives are server-owned: a client cancel request must never end the server's instance.
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
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

	ensureMsgf(HealPerSecond.IsValid(), TEXT("%hs: HealPerSecond is not set on %s"), __FUNCTION__, *GetName());
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealingAuraAbility::Tick(float const DeltaTime)
{
	if (!GeoLib::IsServer(GetWorld()))
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!ensureMsgf(IsValid(Character), TEXT("%hs: invalid Character"), __FUNCTION__))
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = Cast<UGeoAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	IGenericTeamAgentInterface const* OwnerTeamAgent = Cast<IGenericTeamAgentInterface>(Character);
	if (!ensureMsgf(SourceASC && OwnerTeamAgent, TEXT("%hs: invalid ASC or OwnerTeamAgent"), __FUNCTION__))
	{
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


		FHealEffectData HealEffect;
		HealEffect.Amount = HealPerSecond.GetValueAtLevel(GetAbilityLevel()) * DeltaTime;
		HealEffect.bLimitGameplayCue = true;
		UGeoAbilitySystemLibrary::ApplySingleEffectData(HealEffect, SourceASC, TargetASC, GetAbilityLevel(),
														StoredPayload.Seed, StoredPayload.AbilityTag);
		UGeoAbilitySystemLibrary::NotifyAbilityHit(StoredPayload, Actor);
	}
}
