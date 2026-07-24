// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoDashAbility.generated.h"

class UAbilityTask_ApplyRootMotionMoveToForce;

/**
 * Dash ability shared by all player classes.
 *
 * Drives the dash with UAbilityTask_ApplyRootMotionMoveToForce so the root motion source is
 * replicated by ID from the server and reconciled through the CMC saved-move system. The task
 * owns the single authoritative timeout and the finish velocity, so the ability ends off its
 * replicated OnTimedOut on both machines instead of a client-only wall-clock timer that could
 * race ahead of the server and cut the dash short.
 */
UCLASS()
class GEOTRINITY_API UGeoDashAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Sets instancing policy to InstancedPerActor, required for per-ability root motion source ID tracking. */
	UGeoDashAbility();

protected:
	/** Launches the root-motion move task that drives the dash toward the direction encoded in StoredPayload.Yaw. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Ends the ability when the root motion task times out (fires on both server and predicting client). */
	UFUNCTION()
	void OnDashFinished();

	/** Dash direction: current movement direction, falling back to aim yaw when standing still. Computed once on
	 * the activating client and carried in the payload — client and server must build identical root motion
	 * sources (FRootMotionSource_MoveToForce::Matches compares TargetLocation within 0.1cm) or the client can
	 * never reconcile the server's source and receives corrections for the whole dash. */
	virtual float GetFireYaw(AActor const* Instigator, int Seed) const override;

	/** Dash distance in units */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 500.f;

	/** Dash duration in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.2f;
};
