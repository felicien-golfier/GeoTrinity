// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
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

public:
	/** Sets NetSecurityPolicy to ServerOnly so a client cancel request cannot end the server's passive instance after a revive. */
	UGeoHealReturnPassiveAbility();

private:
	/** Binds OnHealProvidedCallback to the owner ASC's OnHealProvided delegate. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	/** Unbinds OnHealProvidedCallback from the owner ASC before calling Super. */
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Called when the owner provides a heal; applies SelfHealPercent of HealDone back to self. */
	UFUNCTION()
	void OnHealProvidedCallback(float HealDone);

	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelfHealPercent = 0.1f;
};
