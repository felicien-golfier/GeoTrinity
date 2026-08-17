// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoEffectZone.generated.h"

class UGeoAbilitySystemComponent;

/**
 * Base class of every zone: an area that applies a configurable set of effects to actors standing inside its capsule.
 * Reached two ways: dropped in a level, where the Details panel describes a zone that lives forever, or spawned by an
 * ability through FDeployableData, which supplies all of that plus a life-drain duration to expire on. The effects and
 * Params.Attitude are what build a healing zone (heal effect, Friendly attitude) or a damage zone (damage effect,
 * Hostile attitude), so a spawned zone needs no Blueprint of its own beyond the project-wide one in
 * UGameDataSettings::DefaultZoneClass — Params.Color is what tells the two apart on screen.
 *
 * Every effect is applied from Tick, never from the overlap delegates: heal/damage entries on each tick, with their
 * magnitude treated as a per-second rate (scaled by DeltaTime), and any other effect type on the first tick after the
 * actor enters, removed when it leaves. Applying from the delegate instead would run a lethal entry's whole
 * death-and-revive chain inside the overlap notification that started it, and a revive re-enables collision — so the
 * zone would re-enter itself until the stack ran out.
 *
 * Subclasses change what a zone does to whoever stands in it by overriding ApplyZoneEffects — the tracking, the
 * capsule, the replicated data and the effect rules above are all inherited (see AGeoHealingZone).
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoEffectZone : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	/** Disables damage on this actor so only its own life drain can end it. */
	AGeoEffectZone(FObjectInitializer const& ObjectInitializer);

	/** Stores the spawner's data: its EffectDataArray and Params replace every Details-panel field. */
	virtual void InitInteractable(FInteractableActorData* InputData) override;
	/** Registers Data (COND_InitialOnly) so clients can size and tint the zone. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual FDeployableData const* GetData() const override { return &Data; }

	/** Sizes and tints the zone so a hand-placed one shows its real extent and colour in the viewport. */
	virtual void OnConstruction(FTransform const& Transform) override;
	/**
	 * Fills Data from the Details panel and self-initializes GAS when no spawner did it (hand-placed zone).
	 * On the server: binds the capsule overlap delegates that track actors inside the zone.
	 */
	virtual void BeginPlay() override;
	/** Server-only: runs ApplyZoneEffects for every actor inside the zone. */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * What the zone does to one actor standing in it, for a single tick: heal/damage entries scaled by DeltaSeconds,
	 * the persistent ones on that actor's first tick inside. Override to give a zone its own rules.
	 *
	 * @param TrackedActor  Key into ActorsInZone — looked up again rather than passed as a raw pointer, since a lethal
	 *                      entry can run its target's whole death and revive from inside this call.
	 */
	virtual void ApplyZoneEffects(TWeakObjectPtr<AActor> const& TrackedActor, UGeoAbilitySystemComponent* SourceASC,
								  float DeltaSeconds);

	/** Actors currently inside the zone, mapped to the persistent effect handles Tick applied to them. An empty array
	 * is what marks an actor as still owed those effects, so a revive inside the zone gets them back. */
	TMap<TWeakObjectPtr<AActor>, TArray<FActiveGameplayEffectHandle>> ActorsInZone;

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	FDeployableData Data;

private:
	// Hand-placed zones only — a spawned zone reads all six off the FDeployableData its ability filled in.
	/** Effects applied to every matching actor: heal/damage tick per second, others persist while inside. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone")
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;
	/** Team this zone belongs to; drives the attitude check against overlapping actors. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone")
	ETeam Team = ETeam::Neutral;
	/** Radius of the zone in world units. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone", meta = (ClampMin = "0.0"))
	float Radius = 200.f;
	/** Effect level used when applying the effects. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone", meta = (ClampMin = "1"))
	int32 Level = 1;
	/** Colour this zone draws in. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone")
	FGeoColorParam Color;
	/** Which attitudes (relative to the zone's team) receive the effects. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 AttitudeBitmask = TeamAttitudeMask::All;

	/** Vector parameters of the zone mesh's material the zone colour is written to. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoEffectZone")
	TArray<FName> ColorParameterNames = {TEXT("InsideColor"), TEXT("OutlineColor")};

	/** Starts tracking OtherActor so Tick applies the zone's effects to it. Both ways in — the overlap delegate and
	 * the sweep BeginPlay runs for whoever the zone spawned on top of — go through here. */
	void EnterZone(AActor* OtherActor);

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex, bool bFromSweep, FHitResult const& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
					  int32 OtherBodyIndex);

	/** Client-side sizing and tinting: a spawned zone's Data only arrives after the actor exists. */
	UFUNCTION()
	void OnRep_Data();

	/** Matches the capsule to Data.Params.Size — the one radius both the placed and the spawned path end up in. */
	void ApplyRadius() const;
	/** Writes Data.Params.Color into ColorParameterNames on every mesh of the zone. */
	void ApplyColor() const;
};
