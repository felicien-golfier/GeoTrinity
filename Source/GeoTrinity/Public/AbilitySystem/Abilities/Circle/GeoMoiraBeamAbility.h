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

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return IsInstantiated() && IsActive() && bIsBeamActive; }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UGeoMoiraBeamAbility, STATGROUP_Tickables);
	}

	bool IsInBeam(AActor const* Actor) const;
#ifdef WITH_EDITOR

	void DrawBeamDebugLines(float DeltaTime) const;
#endif
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat DamagePerSecond;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat HealPerSecond;

	/** Infinite GE applied to self on activation. Should additively modify MovementSpeedMultiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TInstancedStruct<FEffectData> SpeedBuffEffect;

	/** Length of the beam in cm. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float BeamLength = 800.f;

	/** Base beam duration in seconds (before any zone absorption). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float InitialDuration = 3.f;

	/** Fuel added (in seconds) for a full HealingZone consumed by the beam. Scales proportionally with partial
	 * drain. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float DurationPerAbsorbedZone = 2.f;

	/** Beam radius growth (in cm) for a full HealingZone consumed. Scales proportionally with partial drain. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float RadiusGrowthPerAbsorbedZone = 50.f;

	/** Beam Heal And Damage boost for a full HealingZone consumed, 1 will double damage and heal
	 * Scales proportionally with partial drain. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float DamageAndHealBoostPerAbsorbedZone = 1.f;

	/** Health drained from a HealingZone per beam tick. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability",
			  meta = (ClampMin = "0", ClampMax = "100", AllowPrivateAccess = true))
	float BeamZoneDrainPercentagePerSecond = 50.f;

	FActiveGameplayEffectHandle SpeedBuffHandle;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float RemainingDuration = 0.f;
	float BeamRatio = 1.f;
	bool bIsBeamActive = false;
};
