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
 * Deployment is gated by a charge/stack system rather than the live-deployable count: each activation spends one
 * charge and activation is blocked only at zero charges. The charge pool is the stack count of the ability's Cooldown
 * GE — spending applies one stack, and the GE's own RemoveSingleStackAndRefreshDuration expiry hands one back per
 * duration, so a single timer refills the pool sequentially. The pool size is MaxCharges, plus the Surplus
 * Core's magnitude while it is slotted; the GE's own StackLimitCount must stay unlimited (0) or above it.
 */
UCLASS()
class GEOTRINITY_API UGeoDeployAbility : public UGeoProjectileAbility
{
	GENERATED_BODY()

public:
	/** Sets FireMode to ChargeForFireDelay (hold-to-charge, release-to-deploy) and DoNotAutoCommit (stacks drive commit). */
	UGeoDeployAbility();

	/** Returns the deployable class this ability spawns. Used by the HUD to resolve the matching deployable-manager slot. */
	TSubclassOf<AGeoDeployableBase> GetDeployableActorClass() const { return DeployableActorClass; }

	/** Number of charges currently available to spend: the pool size minus the Cooldown GE's replicated stack count. */
	UFUNCTION(BlueprintPure, Category = "GeoAbility|Deploy")
	int32 GetCurrentStacks() const;

	/** Maximum number of charges this ability can hold: MaxCharges, plus the Surplus Core's magnitude with it slotted. */
	UFUNCTION(BlueprintPure, Category = "GeoAbility|Deploy")
	int32 GetMaxStacks() const;

	/** The charges the pool holds without gems: MaxCharges. */
	int32 GetBaseMaxStacks() const
	{
		return MaxCharges;
	}

	/** World location the deployable would land at if the input were released now. Polled every tick by
	 * AGeoDeployTargetCue, which receives this ability instance as its SourceObject. */
	UFUNCTION(BlueprintPure, Category = "GeoAbility|Deploy")
	FVector GetPendingDeployLocation() const;

	/**
	 * Server: spawns the deployable of the deploy ability DeployerASC holds at Location, owned by DeployerASC's owner,
	 * with the deployer's gems on it, without Leverage, which needs a throw. Spends no charge and passes no spawner
	 * projectile. HealthFraction scales its health on top. Null when DeployerASC holds no deploy ability.
	 */
	static AGeoDeployableBase* SpawnDeployableAt(UAbilitySystemComponent& DeployerASC, FVector const& Location,
												 float HealthFraction = 1.f);

protected:
	/** Binds the cooldown-tag event that plays the charge-refilled sound. */
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Unbinds the cooldown-tag event. */
	virtual void OnRemoveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Spends one charge by applying a stack of the Cooldown GE, then runs the normal fire flow. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Blocks activation only when no charges remain (plus the base death check). The alive-deployable count no longer gates. */
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									FGameplayTagContainer const* SourceTags = nullptr,
									FGameplayTagContainer const* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Always true: the Cooldown GE is repurposed as the charge pool, so its tag must not gate activation. */
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							   FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Builds target data encoding the deploy distance (derived from charge ratio) in the Seed field as integer cm. */
	virtual FGeoAbilityTargetData GetUpdatedTargetData() override;

	/** Also adds/removes the local-only DeployTargetCue that marks where the deployable will land. */
	virtual void SetChargeGaugeVisible(APlayableCharacter* Character, bool bVisible) override;

	// LifeDrainMaxDuration is used to define the life drain rate base on "How long the deployable would stay alive in
	// sec if nothing else deplete its life", Size is the DeployableSize, for example it is used by the HealingZone to
	// determine the size of the deployable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|Deploy")
	FDeployableDataParams Params;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility")
	TSubclassOf<AGeoDeployableBase> DeployableActorClass;

	/** Charges the pool holds without gems. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoAbility|Deploy", meta = (ClampMin = "1"))
	int32 MaxCharges = 3;

private:
	/** Plays the charge-refilled sound when the Cooldown GE's stack count drops a charge back into the pool. */
	void OnCooldownTagChanged(FGameplayTag CooldownTag, int32 NewCount);

	/** Deploy distance for the current charge ratio, lerped between the project-wide Min/MaxDeployDistance, before
	 *  the Reach gems' SpellDistanceMultiplier, which the spawner projectile applies to its flight like any shot's. */
	float GetChargedDeployDistance() const;

	/** BaseParams as ASC's gems change them: blink time, drain and health. */
	static FDeployableDataParams ApplyGemsToParams(UAbilitySystemComponent const& ASC,
												   FDeployableDataParams const& BaseParams);

	/** Params as the deployer's gems change them, and Leverage with DeployDistance. */
	FDeployableDataParams GetGemParams(float DeployDistance) const;

	/** Spawns the deployable spawner projectile and, on the deployer's machine, the DeployTargetCue that marks its
	 * landing point until it lands. */
	virtual void SpawnProjectile(FTransform const& SpawnTransform, float SpawnServerTime) const override;

	int32 LastKnownStacks = 0;
	FDelegateHandle CooldownTagDelegateHandle;
};
