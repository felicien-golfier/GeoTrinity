// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "Tickable.h"

#include "GeoHealingAuraAbility.generated.h"

/**
 * Passive healing aura for the Circle player.
 * Periodically heals allies in physical contact (overlapping the character capsule).
 */
UCLASS()
class GEOTRINITY_API UGeoHealingAuraAbility
	: public UGeoGameplayAbility
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Sets NetSecurityPolicy to ServerOnly so a client cancel request cannot end the server's passive instance after a revive. */
	UGeoHealingAuraAbility();

private:
	/** Begins the aura; healing is sustained by the FTickableGameObject tick for as long as the ability is active. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Scans for allies overlapping the avatar's capsule and applies a per-second heal scaled to DeltaTime. */
	virtual void Tick(float DeltaTime) override;
	/** Returns true while the ability is active, keeping the aura heal tick running. */
	virtual bool IsTickable() const override { return IsInstantiated() && IsActive(); }
	/** Returns the stat id for profiling this ability's tick in Unreal's stats system. */
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoHealingAuraAbility, STATGROUP_Tickables);
	}

	/** Heal applied per second to each overlapping ally. Scales with ability level via the curve table row. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat HealPerSecond;
};
