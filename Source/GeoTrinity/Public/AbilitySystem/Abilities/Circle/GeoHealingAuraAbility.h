// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoHealingAuraAbility.generated.h"

/**
 * Passive healing aura for the Circle player.
 * Periodically heals allies in physical contact (overlapping the character capsule).
 * Self-heals once per healed ally per tick — configure SelfHealEffectDataInstances as SelfHealPercent * AuraHealAmount.
 */
UCLASS()
class GEOTRINITY_API UGeoHealingAuraAbility : public UGameplayAbility
{
	GENERATED_BODY()

	virtual void OnAvatarSet(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	void TickAura();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects")
	TInstancedStruct<FEffectData> Heal;

	FTimerHandle AuraTickHandle;
};
