// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "Tickable.h"

#include "GeoHealingAuraAbility.generated.h"

/**
 * Passive healing aura for the Circle player.
 * Periodically heals allies in physical contact (overlapping the character capsule).
 * Self-heals once per healed ally per tick — configure SelfHealEffectDataInstances as SelfHealPercent * AuraHealAmount.
 */
UCLASS()
class GEOTRINITY_API UGeoHealingAuraAbility
	: public UGeoGameplayAbility
	, public FTickableGameObject
{
	GENERATED_BODY()

	virtual void OnAvatarSet(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return IsInstantiated() && IsActive(); }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoHealingAuraAbility, STATGROUP_Tickables);
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat HealPerSecond;
};
