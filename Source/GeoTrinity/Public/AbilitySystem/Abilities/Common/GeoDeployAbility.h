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
 * stack, a single shared timer (the ability's Cooldown GE) refills one stack at a time while below MaxStacks, and
 * activation is blocked only at zero stacks. Fully client-predicted — the stack count is driven off the predicted
 * cooldown tag on both client and server.
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

	/** Number of charges currently available to spend. */
	UFUNCTION(BlueprintPure, Category = "Ability|Deploy")
	int32 GetCurrentStacks() const { return CurrentStacks; }

	/** Maximum number of charges this ability can hold. */
	UFUNCTION(BlueprintPure, Category = "Ability|Deploy")
	int32 GetMaxStacks() const { return MaxStacks; }

protected:
	/** Initializes CurrentStacks to full and binds the cooldown-tag event that refills a stack when the cooldown expires. */
	virtual void OnGiveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Unbinds the cooldown-tag event. */
	virtual void OnRemoveAbility(FGameplayAbilityActorInfo const* ActorInfo, FGameplayAbilitySpec const& Spec) override;

	/** Spends one stack, starts the refill cooldown if it is not already running, then runs the normal fire flow. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Blocks activation only when no stacks remain (plus the base death check). The alive-deployable count no longer gates. */
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
									FGameplayTagContainer const* SourceTags = nullptr,
									FGameplayTagContainer const* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Always true: the Cooldown GE is repurposed as the stack-refill clock, so its tag must not gate activation. */
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

	/** Maximum charges. Each activation spends one; the Cooldown GE refills one at a time while below this. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Deploy", meta = (ClampMin = "1"))
	int32 MaxStacks = 3;

private:
	/** Refills a stack when the cooldown tag clears, re-arming the cooldown while still below MaxStacks. */
	void OnCooldownTagChanged(FGameplayTag CooldownTag, int32 NewCount);

	virtual void SpawnProjectile(FTransform const& SpawnTransform, float SpawnServerTime) const override;

	int32 CurrentStacks = 0;
	FDelegateHandle CooldownTagDelegateHandle;
};
