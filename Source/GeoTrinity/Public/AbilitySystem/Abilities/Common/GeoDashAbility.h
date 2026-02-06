#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "GeoDashAbility.generated.h"

/**
 * Dash ability shared by all player classes.
 * Applies a velocity impulse in the aim direction.
 * Does NOT provide invincibility - character can still take damage during dash.
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
	FVector GetDashDirection(FGameplayAbilityActorInfo const* ActorInfo,
							 UCharacterMovementComponent const* MovementComponent) const;

	FTimerHandle DashTimerHandle;
};
