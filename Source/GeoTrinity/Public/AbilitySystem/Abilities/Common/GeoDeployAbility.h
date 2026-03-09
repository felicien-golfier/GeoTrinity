// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h"
#include "CoreMinimal.h"

#include "GeoDeployAbility.generated.h"

/**
 * Hold to charge, release to deploy a projectile at a distance proportional to charge duration.
 * Intended for deployable spawner projectiles (turrets, etc.) shared across player classes.
 * Deploy distance is encoded in the target data Seed field (as integer cm) for server replication.
 */
UCLASS()
class GEOTRINITY_API UGeoDeployAbility : public UGeoProjectileAbility
{
	GENERATED_BODY()

public:
	UGeoDeployAbility();

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	virtual void EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	virtual void InputReleased(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							   FGameplayAbilityActivationInfo const ActivationInfo) override;

	virtual FGeoAbilityTargetData BuildAbilityTargetData() override;

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Deploy")
	void OnChargeBegin(float MaxChargeSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Deploy")
	void OnChargeEnded();

	UFUNCTION(BlueprintPure, Category = "Ability|Deploy")
	float GetChargeRatio() const;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Deploy")
	float MinDeployDistance = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Deploy")
	float MaxDeployDistance = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Deploy")
	float MaxChargeTime = 2.f;

private:
	void SpawnDeployProjectile(FVector const& Origin, float Yaw, float SpawnServerTime, float DeployDistance) const;
	UFUNCTION()
	void FireDeployable();

	float ChargeStartTime = 0.f;
	float PendingDeployDistance = 0.f;
	FTimerHandle ChargeAutoFireHandle;
};
