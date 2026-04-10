// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GeoGameplayAbility.h"

#include "PatternAbility.generated.h"

/**
 * Server-driven ability that spawns a bullet pattern via multicast RPC.
 * Activates a UPattern instance on all clients, waits for it to finish, then ends the ability.
 * Used exclusively by enemy characters.
 */
UCLASS()
class GEOTRINITY_API UPatternAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;
	UFUNCTION()
	void OnPatternEnd();

public:
	/** Returns the UPattern subclass that this ability will instantiate when activated. */
	TSubclassOf<UPattern> GetPatternClass() const { return PatternToLaunch; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Pattern")
	TSubclassOf<UPattern> PatternToLaunch;
};
