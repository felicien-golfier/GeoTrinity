// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Tickable.h"

#include "GeoMoiraBeamAbility.generated.h"

/**
 * Fire-and-forget beam ability for the Circle player.
 * Fires on activation and sustains independently of button input until fuel is depleted.
 * Damages enemies and heals allies in a cylinder in front of the character.
 * Absorbs deployed HealingZones in the beam path: each absorbed zone adds fuel and grows the beam radius.
 * Applies a movement speed buff for the duration of the channel.
 */
UCLASS()
class GEOTRINITY_API UGeoMoiraBeamAbility
	: public UGeoGameplayAbility
	, public FTickableGameObject
{
	GENERATED_BODY()

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return IsInstantiated() && IsActive(); }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoMoiraBeamAbility, STATGROUP_Tickables);
	}

	bool IsInBeam(AActor const* Actor) const;

#ifdef WITH_EDITOR

	void DrawBeamDebugLines(float DeltaTime) const;
#endif
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects")
	TInstancedStruct<FEffectData> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects")
	TInstancedStruct<FEffectData> HealEffect;

	/** Infinite GE applied to self on activation. Should additively modify MovementSpeedMultiplier. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Effects")
	TInstancedStruct<FEffectData> SpeedBuffEffect;

	/** Length of the beam in cm. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	float BeamLength = 800.f;

	/** Base beam duration in seconds (before any zone absorption). */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	float InitialDuration = 3.f;

	/** Fuel added (in seconds) when a full HealingZone is consumed by the beam. Scales proportionally with partial
	 * drain. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	float DurationPerAbsorbedZone = 2.f;

	/** Beam radius growth (in cm) when a full HealingZone is consumed. Scales proportionally with partial drain. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	float RadiusGrowthPerZone = 50.f;

	/** Health drained from a HealingZone per beam tick. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0", ClampMax = "100"))
	float BeamZoneDrainPercentagePerSecond = 50.f;

	FActiveGameplayEffectHandle SpeedBuffHandle;

	float RemainingDuration = 0.f;
	float AccumulatedRadiusBonus = 0.f;
};
