// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "Camera/CameraShakeBase.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoAutomaticFireAbility.generated.h"

/**
 * Base class for high fire rate abilities that continuously execute while input is held.
 * Uses the base class FireDelay and ScheduleFireTrigger for timing and network compensation.
 * Subclasses override ExecuteShot() to define what happens each shot.
 */
UCLASS(Abstract)
class GEOTRINITY_API UGeoAutomaticFireAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Sets instancing policy to InstancedPerActor, enables replication, and configures CommitBehaviour to DoNotAutoCommit — cost is committed per shot inside Fire(). */
	UGeoAutomaticFireAbility();

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	virtual void EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/**
	 * Auto-fire has no real GAS cooldown, so it reports the per-shot fire-delay timer as the cooldown. This drives
	 * the ability-bar sweep to fill/deplete once per shot instead of pinning grayed for the whole hold.
	 */
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle,
													 FGameplayAbilityActorInfo const* ActorInfo, float& TimeRemaining,
													 float& CooldownDuration) const override;

	virtual void InputReleased(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							   FGameplayAbilityActivationInfo const ActivationInfo) override;

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	/**
	 * Called every shot to perform the actual shot logic (e.g. spawn a projectile).
	 * Override ExecuteShot_Implementation in C++ subclasses; override ExecuteShot in Blueprint.
	 *
	 * @return  True if the shot was executed successfully. False ends the ability immediately.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Ability|AutoFire")
	bool ExecuteShot();
	virtual bool ExecuteShot_Implementation();

	/** Advances to the next fire section when the montage has Fire sections; resets to 0 when it does not. */
	virtual int32& GetFireSectionIndex(UGeoAbilitySystemComponent* ASC, UAnimInstance const* AnimInstance) override;

	/** Camera shake played on the local client each shot. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|GameFeel")
	TSubclassOf<UCameraShakeBase> FireCameraShakeClass;
	/** How far (cm) the mesh snaps backward on each shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|GameFeel", meta = (ClampMin = "0"))
	float RecoilDistance = 12.f;
	/** GameplayCue executed locally on the shooting client each shot. Use for muzzle flash, fire sound, etc. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|GameFeel", meta = (Categories = "GameplayCue"))
	FGameplayTag FireGameplayCueTag;

	int32 CurrentShotIndex = 0;

private:
	bool bWantsToFire = true;
	/** Server-side shot clock: earliest world time the next client-requested shot may execute. */
	float NextAllowedShotTime = 0.f;

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;
};
