#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoDashAbility.generated.h"

/**
 * Dash ability shared by all player classes.
 * Applies a velocity impulse in the aim direction.
 * Does NOT provide invincibility - character can still take damage during dash.
 *
 * Networking: direction and start time are sent in the activation target data so both
 * client and server execute the identical dash. The server trims the timer by the
 * elapsed server time to end in sync despite ping.
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

	/** Dash distance in units */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 500.f;

	/** Dash duration in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.2f;

private:
	FTimerHandle DashTimerHandle;
};
