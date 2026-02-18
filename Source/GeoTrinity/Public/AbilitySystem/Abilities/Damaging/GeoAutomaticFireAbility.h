// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AbilityPayload.h"
#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoAutomaticFireAbility.generated.h"

/**
 * Base class for high fire rate abilities that continuously execute while input is held.
 * Uses the base class FireRate and ScheduleFireTrigger for timing and network compensation.
 * Subclasses override ExecuteShot() to define what happens each shot.
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

	virtual void Fire() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Ability|AutoFire")
	bool ExecuteShot();
	virtual bool ExecuteShot_Implementation();

	float GetShotServerTime(int32 ShotIndex) const;
	void UpdatePayload();

	FAbilityPayload StoredPayload;
	int32 CurrentShotIndex = 0;

private:
	bool bWantsToFire = true;
};
