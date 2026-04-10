// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilityPayload.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "GeoGameplayAbility.generated.h"


struct FGeoAbilityTargetData;
struct FAbilityPayload;
class UEffectDataAsset;
class UGeoAbilitySystemComponent;
class UPattern;

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	ShootAfterFireDelay,
	ChargeForFireDelay,
};

/**
 * Base gameplay ability for GeoTrinity. Provides shared fire-trigger scheduling, animation montage handling,
 * client-to-server target data dispatch, and effect data aggregation used by all derived ability types.
 */
UCLASS()
class GEOTRINITY_API UGeoGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Returns the primary gameplay tag identifying this ability (first owned tag under "Ability"). */
	FGameplayTag GetAbilityTag() const;

	/**
	 * Constructs an FAbilityPayload from explicit shot parameters.
	 *
	 * @param Owner             Actor that owns the ability.
	 * @param Instigator        Actor causing the damage/effect (may differ from Owner).
	 * @param Origin            2D world position of the fire socket at the moment of firing.
	 * @param Yaw               Character facing yaw at the moment of firing (degrees).
	 * @param ServerSpawnTime   Synchronized server time at which the projectile was spawned.
	 * @param Seed              Random seed for deterministic effects.
	 */
	FAbilityPayload CreateAbilityPayload(AActor* Owner, AActor* Instigator, FVector2D const& Origin, float Yaw,
										 float ServerSpawnTime, int Seed) const;

	/**
	 * Constructs an FAbilityPayload from an actor transform.
	 * Derives Origin and Yaw from Transform. ServerSpawnTime is set to current server time.
	 */
	FAbilityPayload CreateAbilityPayload(AActor* Owner, AActor* Instigator, FTransform const& Transform) const;

	/** Returns the GeoAbilitySystemComponent from the ability's current actor info. */
	UGeoAbilitySystemComponent* GetGeoAbilitySystemComponentFromActorInfo() const;

	/** Merges EffectDataAssets and EffectDataInstances into a single flat array for application. */
	virtual TArray<TInstancedStruct<FEffectData>> GetEffectDataArray() const;

	/**
	 * Returns the cooldown duration in seconds for the given ability level.
	 *
	 * @param level  Ability level to evaluate the cooldown at. Defaults to 1.
	 */
	float GetCooldown(int32 level = 1) const;

	virtual void EndAbility(FGameplayAbilitySpecHandle const Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo const ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Convenience overload that ends this ability instance without requiring handle/actorinfo parameters. */
	void EndAbility(bool bReplicateEndAbility = true, bool bWasCancelled = false);

	/**
	 * Returns the normalized charge progress (0–1) based on time held since activation.
	 * Only meaningful when FireMode == EFireMode::ChargeForFireDelay.
	 */
	float GetChargeRatio() const;
	virtual void InputReleased(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							   FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	/**
	 * Starts a timer (or charge window) after which BuildDataAndFire is called.
	 * Duration is FireDelay for ShootAfterFireDelay mode, or unbounded for charge mode (fires on input release).
	 */
	void ScheduleFireTrigger(FGameplayAbilityActivationInfo const& ActivationInfo, UAnimInstance* AnimInstance);
	/**
	 * Selects or advances the animation montage's fire section index.
	 * Override in subclasses that cycle through multiple fire animations.
	 */
	virtual void InitFireSectionIndex(UAnimInstance* AnimInstance, int32& FireSectionIndex);
	/** Plays AnimMontage and jumps to the correct section based on the current fire section index. */
	void HandleAnimationMontage(UAnimInstance* AnimInstance, FGameplayAbilityActivationInfo const& ActivationInfo);
	/** Sends AbilityTargetData to the server via ServerSetReplicatedTargetData for authoritative shot execution. */
	void SendFireDataToServer(FGeoAbilityTargetData const& AbilityTargetData) const;
	/** Builds the FGeoAbilityTargetData for the current shot from character position and facing. Override to customize. */
	virtual FGeoAbilityTargetData BuildAbilityTargetData();
	/** Returns the world location of the ability's fire socket on the character's mesh. */
	FVector GetFireSocketLocation() const;

	/** Timer callback that calls BuildAbilityTargetData, sends it to the server, and calls Fire on the client. */
	UFUNCTION()
	void BuildDataAndFire();

	/** Client-side shot logic (spawn predicted projectile, play VFX). Override in subclasses. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData);

	/**
	 * Server-side shot logic. Called when the server receives target data from the client.
	 * Override to spawn the authoritative projectile or apply server-side effects.
	 */
	virtual void OnFireTargetDataReceived(FGameplayAbilityTargetDataHandle const& DataHandle,
										  FGameplayTag ApplicationTag);

	/** Returns the maximum charge time for charge-mode abilities. Override to return the configured value. */
	UFUNCTION(BlueprintCallable)
	virtual float GetMaxChargeTime() const;


public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> AnimMontage;

	// We consider the ability to Fire at the end of the FireDelay delay.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0", UIMin = "0"))
	float FireDelay = 0.5f;

	// Define the way the ability start to Fire. After a charge time or directly after FireDelay.
	// When FireMode = Charge, we use FireDelay as max charge time.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EFireMode FireMode = EFireMode::ShootAfterFireDelay;

protected:
	FAbilityPayload StoredPayload;
	bool bCommitAtActivate = true;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Effects", meta = (AllowPrivateAccess = true))
	TArray<TInstancedStruct<FEffectData>> EffectDataInstances;
	FTimerHandle FireTriggerTimerHandle;
	float ChargeStartTime = 0.f;
};
