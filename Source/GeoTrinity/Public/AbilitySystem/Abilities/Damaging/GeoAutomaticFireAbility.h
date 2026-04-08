// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "Camera/CameraShakeBase.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoAutomaticFireAbility.generated.h"

/**
 * Base class for high fire rate abilities that continuously execute while input is held.
 * Uses the base class FireDuration and ScheduleFireTrigger for timing and network compensation.
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

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Ability|AutoFire")
	bool ExecuteShot();
	virtual bool ExecuteShot_Implementation();

	virtual void InitFireSectionIndex(UAnimInstance* AnimInstance, int32& FireSectionIndex) override;

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

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;
};
