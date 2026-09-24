// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoDashAbility.generated.h"

/**
 * Dash ability shared by all player classes.
 *
 * Only pays the cost and cooldown and hands the dash to UGeoCharacterMovementComponent::RequestDash, which runs it
 * inside the character's moves: client and server start it on the same move and simulate it identically, so the
 * client is never corrected back to where it dashed from.
 */
UCLASS()
class GEOTRINITY_API UGeoDashAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

protected:
	/** Requests the dash along StoredPayload.Yaw from the movement component, then ends. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Dash direction: current movement direction, falling back to aim yaw when standing still. Computed once on
	 * the activating client and carried in the payload, so client and server dash along the exact same yaw. */
	virtual float GetFireYaw(AActor const* Instigator, int Seed) const override;

	/** Dash distance in units */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDash")
	float DashDistance = 500.f;

	/** Dash duration in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoDash")
	float DashDuration = 0.2f;
};
