// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoChannelBeamAbility.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoMoiraBeamAbility.generated.h"


class AGeoHealingZone;
class ACharacter;

/**
 * Fire-and-forget beam ability for the Circle player.
 * Fires on activation and sustains independently of button input until fuel is depleted.
 * Damages enemies and heals allies in a cylinder in front of the character.
 * Absorbs deployed HealingZones in the beam path: each absorbed zone adds fuel and grows the beam width.
 * Applies a movement speed buff for the duration of the channel.
 */
UCLASS()
class GEOTRINITY_API UGeoMoiraBeamAbility : public UGeoChannelBeamAbility
{
	GENERATED_BODY()

	UGeoMoiraBeamAbility();

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Counts down the remaining fuel before delegating the beam scan to Super. */
	virtual void Tick(float DeltaTime) override;
	/** Drains HealingZones on the beam, damages enemies, and heals allies. */
	virtual void TickBeam(float DeltaTime, TArray<AActor*> const& ActorsInLine) override;

	/** Beam half-width in cm: half the capsule radius, grown by HalfWidthGrowthPerAbsorbedZone per absorbed zone. */
	virtual float GetCurrentBeamHalfWidth(ACharacter const* Character) const override;

	/** Damage applied per second to each enemy inside the beam cylinder. Scales with ability level. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat DamagePerSecond;

	/** Heal applied per second to each ally inside the beam cylinder. Scales with ability level. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	FScalableFloat HealPerSecond;

	/** Infinite GE applied to self on activation. Should additively modify MovementSpeedMultiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TInstancedStruct<FEffectData> SpeedBuffEffect;

	/** Base beam duration in seconds (before any zone absorption). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float InitialDuration = 3.f;

	/** Fuel added (in seconds) for a full HealingZone consumed by the beam. Scales proportionally with partial
	 * drain. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float DurationPerAbsorbedZone = 2.f;

	/** Beam width growth (in cm) for a full HealingZone consumed. Scales proportionally with partial drain. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float HalfWidthGrowthPerAbsorbedZone = 50.f;

	/**
	 * Additive boost to damage and heal for a fully absorbed HealingZone.
	 * A value of 1 doubles damage and heal (adds 100% of the base amount). Scales proportionally with partial drain.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float DamageAndHealBoostPerAbsorbedZone = 1.f;

	/** Health drained from a HealingZone per beam tick. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability",
			  meta = (ClampMin = "0", ClampMax = "100", AllowPrivateAccess = true))
	float BeamZoneDrainPercentagePerSecond = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float BoostPerFinishedZone = .2f;
	TArray<TWeakObjectPtr<AGeoHealingZone>> FinishedZones;

	/** Maximum number of HealingZone actors the beam may fully absorb in a single activation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (AllowPrivateAccess = true))
	float MaximumZoneAbsorbed = 6.f;

	FActiveGameplayEffectHandle SpeedBuffHandle;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float RemainingDuration = 0.f;
	float BeamRatio = 1.f;
};
