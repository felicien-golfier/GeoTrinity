// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoDeployAbility.generated.h"

/**
 * Hold to charge, release to deploy a projectile at a distance proportional to charge duration.
 * Intended for deployable spawner projectiles (turrets, etc.) shared across player classes.
 * Deploy distance is encoded in the target data Seed field (as integer cm) for server replication.
 *
 * Deploy uses are banked as charges (stacks): activating consumes one charge immediately, and one charge recharges
 * in the background at a time (ChargeRechargeTime each) up to MaxCharges. This is independent of
 * UGeoDeployableManagerComponent's cap on simultaneously-alive deployables — that cap still applies on top.
 *
 * Charge state is client-predicted, not replicated: ActivateAbility runs symmetrically on both the predicting
 * client and the server (like the base class's cost/cooldown commit), so both sides run the same recharge
 * countdown off their own local world time and naturally stay in sync. The server remains the sole authority on
 * whether an activation actually goes through — a mispredicting client just gets its activation rejected/rolled
 * back the same way a cost or cooldown misprediction would be.
 */
UCLASS()
class GEOTRINITY_API UGeoDeployAbility : public UGeoProjectileAbility
{
	GENERATED_BODY()

public:
	/** Sets FireMode to ChargeForFireDelay (hold-to-charge, release-to-deploy). */
	UGeoDeployAbility();

	/** Returns the deployable class this ability spawns. Used by the HUD to resolve the matching deployable-manager slot. */
	TSubclassOf<AGeoDeployableBase> GetDeployableActorClass() const { return DeployableActorClass; }

	/** Returns the number of deploy charges currently available (0..GetMaxCharges). */
	UFUNCTION(BlueprintPure, Category = "Ability|Deploy|Charges")
	int32 GetCurrentCharges() const { return CurrentCharges; }

	/** Returns the configured maximum number of banked deploy charges. */
	UFUNCTION(BlueprintPure, Category = "Ability|Deploy|Charges")
	int32 GetMaxCharges() const { return MaxCharges; }

	/**
	 * Outputs the time remaining (seconds) until the next charge finishes recharging, and the full per-charge
	 * recharge duration. Both are 0 while already at max charges. Used by the HUD to draw a recharge indicator
	 * alongside the charge count.
	 */
	void GetChargeRechargeTimeRemainingAndDuration(float& OutTimeRemaining, float& OutDuration) const;

protected:
	/** Checks the base GAS cost/cooldown, that a charge is available, that the small per-deploy delay has elapsed,
	 * and that the player's deployable manager has room for another. */
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									FGameplayTagContainer const* SourceTags = nullptr,
									FGameplayTagContainer const* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Consumes one charge (both the predicting client and the server run this symmetrically, like cost/cooldown). */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Fills the charge bank to MaxCharges when the ability is (re)granted. */
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Reports the small per-deploy delay (not a real GAS cooldown) so the ability-bar sweep flashes once per deploy
	 * instead of reflecting an unrelated cooldown effect. */
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle,
													 FGameplayAbilityActorInfo const* ActorInfo, float& TimeRemaining,
													 float& CooldownDuration) const override;

	/** Builds target data encoding the deploy distance (derived from charge ratio) in the Seed field as integer cm. */
	virtual FGeoAbilityTargetData GetUpdatedTargetData() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy")
	float MinDeployDistance = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy")
	float MaxDeployDistance = 1500.f;

	// LifeDrainMaxDuration is used to define the life drain rate base on "How long the deployable would stay alive in
	// sec if nothing else deplete its life", Size is the DeployableSize, for example it is used by the HealingZone to
	// determine the size of the deployable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy")
	FDeployableDataParams Params;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoDeployableBase> DeployableActorClass;

	/** Maximum number of deploy uses banked at once. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy|Charges", meta = (ClampMin = "1"))
	int32 MaxCharges = 3;

	/** Seconds to recharge one charge in the background. Recharges sequentially: only one charge regenerates at a
	 * time even if several are missing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy|Charges", meta = (ClampMin = "0.1"))
	float ChargeRechargeTime = 5.f;

	/** Minimum time between two consecutive deploys, even when charges are available — lets the player spend banked
	 * charges only one at a time instead of all at once. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy|Charges", meta = (ClampMin = "0"))
	float MinTimeBetweenDeploys = 0.4f;

private:
	virtual void SpawnProjectile(FTransform const& SpawnTransform, float SpawnServerTime) const override;

	/** Decrements CurrentCharges and (re)starts the recharge timer if needed. */
	void ConsumeCharge();
	/** Starts the recharge timer if below max and no recharge is already ticking. */
	void StartRechargeTimerIfNeeded();
	/** Timer callback: adds one charge back and restarts the timer if still below max. */
	void OnChargeRecharged();

	/** Current number of banked deploy charges. Not replicated: both the predicting client and the server run their
	 * own recharge countdown off local world time and naturally stay in sync (see class comment). */
	int32 CurrentCharges = 0;

	/** Local world time at which the currently-recharging charge completes; 0 when no recharge is in progress
	 * (already at max charges). */
	float NextChargeReadyWorldTime = 0.f;

	/** Local world time of the last activation, used only to enforce MinTimeBetweenDeploys. */
	float LastActivationWorldTime = -1.f;

	/** Ticks down ChargeRechargeTime for the charge currently recharging. */
	FTimerHandle RechargeTimerHandle;
};
