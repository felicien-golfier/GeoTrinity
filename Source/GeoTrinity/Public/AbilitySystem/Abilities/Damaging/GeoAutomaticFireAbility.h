// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoAutomaticFireAbility.generated.h"

/**
 * Base class for high fire rate abilities that continuously execute while input is held.
 * Subclasses override ExecuteShot() to define what happens each shot (spawn projectiles, apply effects, etc.)
 *
 * Network synchronization strategy (following Lyra's pattern):
 * - Single RPC at ability activation with initial target data (Origin, Yaw, ServerSpawnTime, Seed)
 * - Each shot is deterministically timed: ShotServerTime = InitialServerTime + (ShotIndex * FireInterval)
 * - Both client and server calculate identical shot times, ensuring sync without per-shot RPCs
 *
 * Key features:
 * - bRetriggerInstancedAbility = false: prevents re-activation spam while active
 * - InstancingPolicy = InstancedPerActor: maintains state across firing session
 * - InputReleased() override for input release detection (native ASC callback, no task overhead)
 * - Timer-based firing loop (no task overhead per shot)
 */
UCLASS(Abstract)
class GEOTRINITY_API UGeoAutomaticFireAbility : public UGeoGameplayAbility
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

	/**
	 * Called each time the ability should fire. Subclasses must override to define behavior.
	 * @return true if the shot was successful and firing should continue, false to stop firing
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Ability|AutoFire")
	bool ExecuteShot();
	virtual bool ExecuteShot_Implementation();

	/** Calculate the server time for a specific shot index for deterministic sync */
	float GetShotServerTime(int32 ShotIndex) const;

	/** Update StoredPayload with current avatar position/yaw */
	void UpdatePayloadFromAvatar();

	/** Time between shots in seconds. FireRate = 1/FireInterval */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AutoFire",
			  meta = (ClampMin = "0.01", UIMin = "0.05"))
	float FireInterval = 0.1f;

	/** If true, updates position/yaw from avatar each shot (for moving characters).
	 *  If false, uses initial position only (for stationary turrets, better sync but less responsive). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AutoFire")
	bool bUpdatePositionPerShot = true;

	/** Payload containing origin, yaw, timing and seed for network sync */
	FAbilityPayload StoredPayload;

	/** Current shot index for deterministic timing and seed variation */
	int32 CurrentShotIndex = 0;

private:
	/** Fire a single shot and schedule the next one */
	UFUNCTION()
	void FireShot();

	/** Timer handle for the firing loop */
	FTimerHandle FireTimerHandle;

	/** Track if we should continue firing */
	bool bWantsToFire = true;
};
