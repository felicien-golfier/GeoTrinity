// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Data/EffectData.h"
#include "Actor/GeoInteractableActor.h"
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Tool/Team.h"

#include "GeoEffectZone.generated.h"

/**
 * Editor-placeable area that applies a configurable set of effects to actors standing inside its capsule.
 * Self-contained: owns its ASC (via AGeoInteractableActor) and is configured entirely from the Details panel —
 * no spawner or ability is required. Drop one in a level, set the Team, AttitudeBitmask and EffectDataArray to
 * build healing zones (heal effect, Friendly attitude) or damage zones (damage effect, Hostile attitude).
 *
 * Effects are split by type: heal/damage entries are applied every tick with their magnitude treated as a
 * per-second rate (scaled by DeltaTime). Any other effect type is applied once when the actor enters the zone
 * and removed when it leaves.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoEffectZone : public AGeoInteractableActor
{
	GENERATED_BODY()

public:
	/** Disables damage on this actor so it cannot be destroyed during play. */
	AGeoEffectZone(FObjectInitializer const& ObjectInitializer);

protected:
	/** Returns the interactable data populated from Details-panel properties in OnConstruction and BeginPlay. */
	virtual FInteractableActorData const* GetData() const override { return &Data; }

	/** Pushes Team/Level into Data and updates the capsule dimensions to match Radius. */
	virtual void OnConstruction(FTransform const& Transform) override;
	/**
	 * Self-initializes GAS using this actor as owner (no spawner sets this up for editor-placed actors).
	 * On the server: binds the capsule overlap delegates that track actors inside the zone.
	 */
	virtual void BeginPlay() override;
	/** Server-only: re-applies heal/damage effects (magnitude * DeltaSeconds) to every actor inside the zone. */
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Effects applied to every matching actor: heal/damage tick per second, others persist while inside. */
	UPROPERTY(EditAnywhere, Category = "EffectZone")
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;

	/** Team this zone belongs to; drives the attitude check against overlapping actors. */
	UPROPERTY(EditAnywhere, Category = "EffectZone")
	ETeam Team = ETeam::Neutral;

	/** Which attitudes (relative to Team) receive the effects. */
	UPROPERTY(EditAnywhere, Category = "EffectZone",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 AttitudeBitmask = TeamAttitudeMask::All;

	/** Radius of the zone in world units. */
	UPROPERTY(EditAnywhere, Category = "EffectZone", meta = (ClampMin = "0.0"))
	float Radius = 200.f;

	/** Effect level used when applying the effects. */
	UPROPERTY(EditAnywhere, Category = "EffectZone", meta = (ClampMin = "1"))
	int32 Level = 1;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);

	/** Actors currently inside the zone, mapped to the persistent effect handles applied to them on entry. */
	TMap<TWeakObjectPtr<AActor>, TArray<FActiveGameplayEffectHandle>> ActorsInZone;

	FInteractableActorData Data;
};
