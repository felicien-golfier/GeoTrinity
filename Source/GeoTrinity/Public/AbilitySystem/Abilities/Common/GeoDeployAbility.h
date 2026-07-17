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
 * duration, so a single timer refills the pool sequentially. The pool size is the GE's StackLimitCount.
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
	UFUNCTION(BlueprintPure, Category = "Ability|Deploy")
	int32 GetCurrentStacks() const;

	/** Maximum number of charges this ability can hold — the Cooldown GE's StackLimitCount. */
	UFUNCTION(BlueprintPure, Category = "Ability|Deploy")
	int32 GetMaxStacks() const;

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

	// LifeDrainMaxDuration is used to define the life drain rate base on "How long the deployable would stay alive in
	// sec if nothing else deplete its life", Size is the DeployableSize, for example it is used by the HealingZone to
	// determine the size of the deployable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy")
	FDeployableDataParams Params;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<AGeoDeployableBase> DeployableActorClass;

private:
	/** Plays the charge-refilled sound when the Cooldown GE's stack count drops a charge back into the pool. */
	void OnCooldownTagChanged(FGameplayTag CooldownTag, int32 NewCount);

	virtual void SpawnProjectile(FTransform const& SpawnTransform, float SpawnServerTime) const override;

	int32 LastKnownStacks = 0;
	FDelegateHandle CooldownTagDelegateHandle;
};
