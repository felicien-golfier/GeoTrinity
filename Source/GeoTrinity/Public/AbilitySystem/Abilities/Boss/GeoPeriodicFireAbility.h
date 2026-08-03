// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/PatternAbility.h"
#include "CoreMinimal.h"

#include "GeoPeriodicFireAbility.generated.h"

/**
 * Fires salves at all players, again and again: the ability never ends on its own, every salve schedules the next one,
 * so a single activation keeps the boss firing.
 * One launch of its USalvePattern is one salve, aimed and stamped here right before it leaves — so a salve always
 * departs from where the boss stands at that moment, towards where the players stand, instead of replaying an aim taken
 * at the start of the burst.
 */
UCLASS()
class GEOTRINITY_API UGeoPeriodicFireAbility : public UPatternAbility
{
	GENERATED_BODY()

public:
	/** Configures ServerOnly net execution, InstancedPerActor instancing, and no replication. */
	UGeoPeriodicFireAbility();

	/** Drops what is left of the previous activation's burst — the instance is kept alive between activations. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

protected:
	/** Aims one yaw at each alive player, from the origin the salve about to leave was stamped with. */
	virtual TInstancedStruct<FPatternData> CreatePatternData() const override;

	/** Fires one salve, sizing a new burst first when the previous one is spent, then schedules what comes next: the
	 * next salve of the burst, or the first salve of the next burst. */
	virtual void LaunchPattern() override;

	/** A finished salve ends nothing: the next one was already scheduled when this one left. */
	virtual void OnPatternEnd() override {}

	/** Sets how many salves the next burst holds and how far apart they leave, from the boss health and the party size.
	 */
	void SizeBurst();

	/** Time one round of the burst spans. A full party takes a single salve per round; every player short of it adds
	 * one more salve inside that same window, so the burst carries the same number of projectiles whatever the party
	 * size. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.01"))
	float RoundDuration = .3f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.1"))
	float FireIntervalMin = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (ClampMin = "0.1"))
	float FireIntervalMax = 5.f;

private:
	int32 RemainingSalveCount = 0;
	float TimeBetweenSalves = 0.f;
};
