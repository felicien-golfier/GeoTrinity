// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoInteractableActor.h"
#include "AbilitySystem/Data/EffectData.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/Team.h"

#include "GeoEffectZone.generated.h"

/**
 * Editor-placeable area that periodically applies a configurable set of effects to actors standing inside it.
 * Self-contained: owns its ASC (via AGeoInteractableActor) and is configured entirely from the Details panel —
 * no spawner or ability is required. Drop one in a level, set the Team, AttitudeBitmask and EffectDataArray to
 * build healing zones (heal effect, Friendly attitude) or damage zones (damage effect, Hostile attitude).
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoEffectZone : public AGeoInteractableActor
{
	GENERATED_BODY()

public:
	AGeoEffectZone();

protected:
	virtual FInteractableActorData const* GetData() const override { return &Data; }

	virtual void OnConstruction(FTransform const& Transform) override;
	virtual void BeginPlay() override;

private:
	/** Effects applied to every matching actor each interval (heal, damage, status, …). */
	UPROPERTY(EditAnywhere, Category = "EffectZone")
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	/** Team this zone belongs to; drives the attitude check against overlapping actors. */
	UPROPERTY(EditAnywhere, Category = "EffectZone")
	ETeam Team = ETeam::Neutral;

	/** Which attitudes (relative to Team) receive the effects. */
	UPROPERTY(EditAnywhere, Category = "EffectZone", meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 AttitudeBitmask = TeamAttitudeMask::Hostile;

	/** Radius of the zone in world units. */
	UPROPERTY(EditAnywhere, Category = "EffectZone", meta = (ClampMin = "0.0"))
	float Radius = 200.f;

	/** Seconds between effect applications. */
	UPROPERTY(EditAnywhere, Category = "EffectZone", meta = (ClampMin = "0.05"))
	float TickInterval = 1.f;

	/** Effect level used when applying the effects. */
	UPROPERTY(EditAnywhere, Category = "EffectZone", meta = (ClampMin = "1"))
	int32 Level = 1;

	void ApplyEffectsToActorsInZone();

	FInteractableActorData Data;
	FTimerHandle TickTimerHandle;
};
