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
 */
UCLASS()
class GEOTRINITY_API UGeoDeployAbility : public UGeoProjectileAbility
{
	GENERATED_BODY()

public:
	UGeoDeployAbility();

	/** Returns the current charge progress (0 = empty, 1 = fully charged) after applying the charge curve. */
	UFUNCTION(BlueprintPure, Category = "Ability|Deploy")
	float GetChargeRatio() const;

protected:
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									FGameplayTagContainer const* SourceTags = nullptr,
									FGameplayTagContainer const* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	virtual void EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	virtual void InputReleased(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							   FGameplayAbilityActivationInfo const ActivationInfo) override;

	virtual FGeoAbilityTargetData BuildAbilityTargetData() override;

	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Deploy")
	void OnChargeBegin(float MaxChargeSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Deploy")
	void OnChargeEnded();

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

private:
	void SpawnDeployProjectile(FVector const& Origin, float Yaw, float SpawnServerTime, float DeployDistance) const;
	UFUNCTION()
	void FireDeployable();
	float GetRawChargeRatio() const;
	float ApplyChargeCurve(float RawRatio) const;

	float ChargeStartTime = 0.f;
	float PendingDeployDistance = 0.f;
	FTimerHandle ChargeAutoFireHandle;
};
