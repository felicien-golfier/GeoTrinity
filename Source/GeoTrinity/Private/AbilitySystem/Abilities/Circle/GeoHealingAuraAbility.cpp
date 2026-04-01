// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Circle/GeoHealingAuraAbility.h"

#include "AbilitySystem/AttributeSet/GeoAttributeSetBase.h"
#include "AbilitySystem/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Settings/GameDataSettings.h"
#include "Tool/UGameplayLibrary.h"

// ---------------------------------------------------------------------------------------------------------------------w
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

	if (!UGameplayLibrary::IsServer(GetWorld()))
	{
		return;
	}

	float const AuraTickInterval = GetDefault<UGameDataSettings>()->RegularTickInterval;

	GetWorld()->GetTimerManager().SetTimer(AuraTickHandle, this, &ThisClass::TickAura, AuraTickInterval, true);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealingAuraAbility::EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
										FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
										bool bWasCancelled)
{
	if (UGameplayLibrary::IsServer(GetWorld()))
	{
		GetWorld()->GetTimerManager().ClearTimer(AuraTickHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoHealingAuraAbility::TickAura()
{
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

	int32 AlliesHealed = 0;
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

		UGeoAbilitySystemLibrary::ApplySingleEffectData(Heal, SourceASC, TargetASC, GetAbilityLevel(), 0);
		AlliesHealed++;
	}

	for (int32 i = 0; i < AlliesHealed; i++)
	{
		UGeoAbilitySystemLibrary::ApplySingleEffectData(Heal, SourceASC, SourceASC, GetAbilityLevel(), 0);
	}
}
