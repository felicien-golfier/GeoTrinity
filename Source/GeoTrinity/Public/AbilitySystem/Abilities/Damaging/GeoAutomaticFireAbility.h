// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"
#include "CoreMinimal.h"

#include "GeoAutomaticFireAbility.generated.h"

/**
 * High fire rate ability that continuously spawns projectiles while input is held.
 *
 * Network synchronization strategy (following Lyra's pattern):
 * - Single RPC at ability activation with initial target data (Origin, Yaw, ServerSpawnTime, Seed)
 * - Each shot is deterministically timed: ShotServerTime = InitialServerTime + (ShotIndex * FireInterval)
 * - Both client and server calculate identical shot times, ensuring sync without per-shot RPCs
 *
 * Key features:
 * - bRetriggerInstancedAbility = false: prevents re-activation spam while active
 * - InstancingPolicy = InstancedPerActor: maintains state across firing session
 * - Uses UAbilityTask_WaitInputRelease for GAS-compatible input release detection
 * - Timer-based firing loop (no task overhead per shot)
 */
UCLASS()
class GEOTRINITY_API UGeoAutomaticFireAbility : public UGeoProjectileAbility
{
	GENERATED_BODY()

public:
	UGeoAutomaticFireAbility();

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	virtual void EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	virtual void InputReleased(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							   FGameplayAbilityActivationInfo const ActivationInfo) override;

	/** Fire a single projectile and schedule the next shot */
	UFUNCTION()
	void FireShot();

	/** Called when input is released via WaitInputRelease task */
	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	/** Calculate the server time for a specific shot index for deterministic sync */
	float GetShotServerTime(int32 ShotIndex) const;

	/** Time between shots in seconds. FireRate = 1/FireInterval */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AutoFire",
			  meta = (ClampMin = "0.01", UIMin = "0.05"))
	float FireInterval = 0.1f;

	/** If true, fires immediately on activation. If false, waits FireInterval before first shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AutoFire")
	bool bFireImmediately = true;

	/** If true, commits cost/cooldown only once at start. If false, commits per shot (higher cost for sustained fire)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AutoFire")
	bool bCommitOnceOnly = true;

	/** If true, updates position/yaw from avatar each shot (for moving characters).
	 *  If false, uses initial position only (for stationary turrets, better sync but less responsive). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AutoFire")
	bool bUpdatePositionPerShot = true;

private:
	/** Timer handle for the firing loop */
	FTimerHandle FireTimerHandle;

	/** Current shot index for deterministic timing */
	int32 CurrentShotIndex = 0;

	/** Track if we should continue firing */
	bool bWantsToFire = true;
};
