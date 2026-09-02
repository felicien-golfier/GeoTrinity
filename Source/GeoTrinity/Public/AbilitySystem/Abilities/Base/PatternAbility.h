// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/GeoCueParam.h"
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
	/** Commits cost and cooldown, telegraphs the launch, then launches the pattern PreLaunchDelay seconds later. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Drops the telegraph and force-stops the running pattern before calling Super, so the pattern end never loops
	 * back into this ability. */
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
	UFUNCTION()
	virtual void LaunchPattern();

	/** Server. The pattern finished: ends the ability. Override to launch again instead of ending. */
	UFUNCTION()
	virtual void OnPatternEnd();

	/**
	 * Builds the pattern-specific data sent through PatternStartMulticast to every client's InitPattern.
	 * Base returns an unset struct (no extra data). Override to fill your own FPatternData subclass.
	 */
	virtual TInstancedStruct<FPatternData> CreatePatternData() const { return {}; }

	/**
	 * Server. Runs at activation, PreLaunchDelay seconds before the pattern launches. Base telegraphs the caster
	 * itself; override to telegraph what the pattern is about to hit, and to resolve there — from LaunchSeed — whatever
	 * the launch must not choose a second time.
	 */
	virtual void BeginPreLaunch();

	/**
	 * Marks TargetASC with PreLaunchCue until the pattern launches, at most once per ASC.
	 * An added cue is replicated ASC state, so unlike an executed one it survives packet loss and a late joiner still
	 * sees the telegraph already running.
	 */
	void AddPreLaunchCue(UGeoAbilitySystemComponent* TargetASC);

	/**
	 * Seed the next launch stamps into its payload. Rolled before PreLaunchDelay so the telegraph and the pattern agree
	 * on what was chosen, and re-rolled after every launch so a chained pattern keeps varying.
	 */
	int LaunchSeed{};

private:
	/** Drops PreLaunchCue from every ASC it was added to. */
	void RemovePreLaunchCues();

	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Pattern", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPattern> PatternToLaunch;

	// Telegraph time between the activation and the pattern launch. The ability holds its cooldown for that long too.
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Pattern", meta = (AllowPrivateAccess = "true"))
	float PreLaunchDelay = 0.f;

	// Cue marking what the pattern is about to hit, carrying PreLaunchDelay as RawMagnitude so the notify can size its
	// countdown. Added on activation, removed on launch.
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Pattern", meta = (AllowPrivateAccess = "true"))
	FGeoCueParam PreLaunchCue;

	TArray<TWeakObjectPtr<UGeoAbilitySystemComponent>> PreLaunchCueASCs;

	FTimerHandle PreLaunchTimerHandle;
};
