// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Base/GeoGameplayAbility.h"
#include "AbilitySystem/Data/GeoCueParam.h"
#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoZoneAbility.generated.h"

/**
 * Server-only enemy ability that telegraphs a circle on the ground for its whole FireDelay, then delivers its effects
 * there. A ZoneParams.LifeDrainMaxDuration above zero leaves a zone behind that carries the effects for that long;
 * zero lands them all at once on whoever stands in the circle, which is the same telegraph with a burst instead of a
 * lingering zone. The lingering form needs no Blueprint of its own — ZoneParams.Color tells one zone from another, and
 * the class comes from UGameDataSettings::DefaultZoneClass unless ZoneClass overrides it.
 *
 * The ability owns no rhythm of its own beyond that single telegraph-then-deliver pass: it ends the moment it fires, so
 * how often a zone comes back is the caster's StateTree to decide, not this class.
 */
UCLASS()
class GEOTRINITY_API UGeoZoneAbility : public UGeoGameplayAbility
{
	GENERATED_BODY()

public:
	/** Configures ServerOnly net execution, InstancedPerActor instancing, and no replication. */
	UGeoZoneAbility();

protected:
	/** Runs the normal activation, then telegraphs the circle for the fire delay it just scheduled. */
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo const* ActorInfo,
								 FGameplayAbilityActivationInfo ActivationInfo,
								 FGameplayEventData const* TriggerEventData) override;

	/** Server. Spawns ZoneClass at the telegraphed circle, or bursts the effects there when it is unset, then ends. */
	virtual void Fire(FGeoAbilityTargetData const& AbilityTargetData) override;

	/** Zone left behind by this ability. Unset uses UGameDataSettings::DefaultZoneClass. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Zone")
	TSubclassOf<AGeoDeployableBase> ZoneClass;

	/** Size (the telegraphed radius), colour, blink and life-drain duration of the zone — a zero duration makes the
	 * ability a one-shot burst instead. Size is the burst radius too. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Zone")
	FDeployableDataParams ZoneParams;

	/** Where the circle lands relative to the caster, in its facing frame. Zero drops it on the caster itself. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Zone")
	FVector2D Offset = FVector2D::ZeroVector;

	/** Which attitudes a burst hits. Unused with a ZoneClass — the zone carries its own attitude filter. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Zone",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 BurstAttitude = TeamAttitudeMask::HostileOrNeutral;

	/** Telegraph drawn at the circle for the whole fire delay. Color it after the effect it announces. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Zone")
	FGeoCueParam TelegraphCue;

	/** Played where a burst lands. Unused with a ZoneClass — the zone plays its own spawn cue. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoAbility|Zone")
	FGeoCueParam BurstCue;

private:
	/** ZoneClass, or the project-wide zone from UGameDataSettings when this ability names none. */
	TSubclassOf<AGeoDeployableBase> GetZoneClass() const;

	/** Applies the ability's effects to everything BurstAttitude matches inside the circle, then plays BurstCue. */
	void Burst(FVector const& ZoneLocation) const;

	/** Centre of the circle: the caster's fire origin pushed by Offset along its facing. */
	FVector GetZoneLocation() const;

	/**
	 * Fires Cue on every machine at the circle, carrying its radius and how long the cue has to run.
	 *
	 * @param Duration  Seconds the cue lasts — the remaining fire delay for a telegraph, 0 for a one-frame burst.
	 */
	void ExecuteZoneCue(FGeoCueParam const& Cue, FVector const& ZoneLocation, float Duration) const;
};
