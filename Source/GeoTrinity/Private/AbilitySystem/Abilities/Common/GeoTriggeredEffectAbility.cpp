// Copyright 2024 GeoTrinity. All Rights Reserved.

#include "AbilitySystem/Abilities/Common/GeoTriggeredEffectAbility.h"

#include "AbilitySystem/Components/GeoAbilitySystemComponent.h"
#include "AbilitySystem/Lib/GeoAbilitySystemLibrary.h"
#include "Tool/UGeoGameplayLibrary.h"

UGeoTriggeredEffectAbility::UGeoTriggeredEffectAbility()
{
	// Passives are server-owned: a client cancel request must never end the server's instance.
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTriggeredEffectAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,
												 FGameplayAbilityActorInfo const* ActorInfo,
												 FGameplayAbilityActivationInfo ActivationInfo,
												 FGameplayEventData const* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bIsAbilityEnding)
	{
		return;
	}

	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoTriggeredEffectAbility: invalid ASC on activation")))
	{
		return;
	}

	if (GeoLib::IsServer(GetWorld()))
	{
		SourceASC->OnAbilityHit.AddDynamic(this, &UGeoTriggeredEffectAbility::OnAbilityHitCallback);
		SourceASC->AbilityActivatedCallbacks.AddUObject(this, &UGeoTriggeredEffectAbility::OnAbilityActivatedCallback);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTriggeredEffectAbility::EndAbility(FGameplayAbilitySpecHandle Handle,
											FGameplayAbilityActorInfo const* ActorInfo,
											FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
											bool bWasCancelled)
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (SourceASC && GeoLib::IsServer(GetWorld()))
	{
		SourceASC->OnAbilityHit.RemoveDynamic(this, &UGeoTriggeredEffectAbility::OnAbilityHitCallback);
		SourceASC->AbilityActivatedCallbacks.RemoveAll(this);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTriggeredEffectAbility::OnAbilityHitCallback(FGameplayTag AbilityTag, AActor* HitInstigator,
													  AActor* /*HitActor*/)
{
	// Deployables are spawned with the character that placed them as their pawn instigator, which is what tells a
	// turret's or a mine's shot from the ones the character fired itself.
	bool const bFiredItself = HitInstigator == StoredPayload.SourceAvatar;
	bool const bFiredByOwnedActor =
		!bFiredItself && IsValid(HitInstigator) && HitInstigator->GetInstigator() == StoredPayload.SourceAvatar;

	if (((bFiredItself && HitTriggerSource != EGeoHitTriggerSource::OwnedActor)
		 || (bFiredByOwnedActor && HitTriggerSource != EGeoHitTriggerSource::Instigator))
		&& HitTriggerTags.HasTag(AbilityTag) != bInvertHitTriggerTags)
	{
		ApplyEffectsToSelf();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTriggeredEffectAbility::OnAbilityActivatedCallback(UGameplayAbility* Ability)
{
	if (ActivationTriggerTags.HasTag(GeoASLib::GetAbilityTagFromAbility(*Ability)))
	{
		ApplyEffectsToSelf();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
void UGeoTriggeredEffectAbility::ApplyEffectsToSelf()
{
	UGeoAbilitySystemComponent* SourceASC = GetGeoAbilitySystemComponentFromActorInfo();
	if (!ensureMsgf(SourceASC, TEXT("UGeoTriggeredEffectAbility: invalid ASC when applying triggered effects")))
	{
		return;
	}

	GeoASLib::ApplyEffectFromEffectData(GetEffectDataArray(), SourceASC, SourceASC, GetAbilityLevel(),
										StoredPayload.Seed, StoredPayload.AbilityTag);
}
