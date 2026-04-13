// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/GeoGameplayAbility.h"
#include "CoreMinimal.h"

#include "GeoHealReturnPassiveAbility.generated.h"

/**
 * Passive ability for the Circle player.
 * Listens to OnHealProvided on the owner's ASC and heals self for a percent of heals dealt.
 */
UCLASS()
class GEOTRINITY_API UGeoHealReturnPassiveAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	UFUNCTION()
	void OnHealProvidedCallback(float HealDone);

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelfHealPercent = 0.1f;
};
