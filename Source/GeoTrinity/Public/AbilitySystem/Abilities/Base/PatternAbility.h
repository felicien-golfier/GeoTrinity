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

public:
	/** Commits cost and cooldown, then launches the pattern. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Force-stops the running pattern before calling Super, so the pattern end never loops back into this ability. */
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
							FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
							bool bWasCancelled) override;

	/** Returns the UPattern subclass that this ability will instantiate when activated. */
	TSubclassOf<UPattern> GetPatternClass() const { return PatternToLaunch; }

protected:
	/**
	 * Stamps a fresh payload, multicasts the pattern to every machine and binds its end delegate.
	 * Every launch re-syncs the pattern on the server time of that moment, so an ability may launch more than once.
	 * Override to chain the next launch.
	 */
	virtual void LaunchPattern();

	/** Server. The pattern finished: ends the ability. Override to launch again instead of ending. */
	UFUNCTION()
	virtual void OnPatternEnd();

	/**
	 * Builds the pattern-specific data sent through PatternStartMulticast to every client's InitPattern.
	 * Base returns an unset struct (no extra data). Override to fill your own FPatternData subclass.
	 */
	virtual TInstancedStruct<FPatternData> CreatePatternData() const { return {}; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Pattern", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPattern> PatternToLaunch;
};
