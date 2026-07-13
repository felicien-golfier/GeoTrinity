// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoDashAbility.generated.h"

/**
 * Dash ability shared by all player classes.
 *
 * Uses FRootMotionSource_MoveToForce so the dash movement is part of the CMC saved-move
 * system. This means the server replays client moves with the exact same root motion
 * applied, avoiding the CMC position-correction artifacts that LaunchCharacter causes
 * (LaunchCharacter is not saved in FSavedMove_Character, so server/client velocities
 * diverge and the CMC sends corrections that manifest as snap-teleport artefacts).
 */
UCLASS()
class GEOTRINITY_API UGeoDashAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	UGeoDashAbility();

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Dash direction: current movement direction, falling back to aim yaw when standing still. Computed once on
	 * the activating client and carried in the payload — client and server must build identical root motion
	 * sources (FRootMotionSource_MoveToForce::Matches compares TargetLocation within 0.1cm) or the client can
	 * never reconcile the server's source and receives corrections for the whole dash. */
	virtual float GetFireYaw(AActor const* Instigator) const override;

	/** Dash distance in units */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 500.f;

	/** Dash duration in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.2f;

private:
	uint16 DashRootMotionSourceID{0};
	FTimerHandle DashEndTimerHandle;
};
