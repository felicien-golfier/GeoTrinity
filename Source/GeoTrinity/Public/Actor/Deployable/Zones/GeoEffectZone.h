// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/Deployable/GeoDeployableBase.h"
#include "CoreMinimal.h"

#include "GeoEffectZone.generated.h"

class UGeoAbilitySystemComponent;
class UGeoZoneVisualComponent;

/**
 * Base class of every zone: an area that applies a configurable set of effects to actors standing inside its capsule.
 * Reached two ways: dropped in a level, where the Details panel describes a zone that lives forever, or spawned by an
 * ability through FDeployableData, which supplies all of that plus a life-drain duration to expire on. The effects and
 * Params.Attitude are what build a healing zone (heal effect, Friendly attitude) or a damage zone (damage effect,
 * Hostile attitude), so a spawned zone needs no Blueprint of its own beyond the project-wide one in
 * UGameDataSettings::DefaultZoneClass — Params.Color is what tells the two apart on screen, drawn by ZoneVisual on the
 * zone layer (World/ZoneLayer/CLAUDE.md). Params.GrowToSize above 0
 * grows it from Size over Params.GrowSeconds, then it stays: every machine grows it on the server clock from the
 * server's BeginPlay (GrowthStartServerTime), so a client that receives the zone late draws the radius the server
 * judges.
 *
 * The server judges the zone as a hazard (FGeoHazardJudge), each actor where it stood when its screen showed it: the
 * per-second entries for the time spent inside, any other entry on entering, its infinite ones removed on leaving.
 * The zone stops acting once it blinks or expires — a placed one never does.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class GEOTRINITY_API AGeoEffectZone : public AGeoDeployableBase
{
	GENERATED_BODY()

public:
	/** Disables damage on this actor so only its own life drain can end it; creates ZoneVisual; Color starts Neutral.
	 */
	AGeoEffectZone(FObjectInitializer const& ObjectInitializer);

	/** Stores the spawner's data: its EffectDataArray and Params replace every Details-panel field. */
	virtual void InitInteractable(FInteractableActorData* InputData) override;
	/** Registers Data and GrowthStartServerTime (COND_InitialOnly) so clients can size, grow and tint the zone. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** Advances the growth (Data.Params.GrowToSize above 0) and resizes the zone; fills ZoneVisual with the life spent.
	 */
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual FDeployableData const* GetData() const override { return &Data; }

	/** Sizes and colours the zone so a hand-placed one shows its real extent and colours in the viewport. */
	virtual void OnConstruction(FTransform const& Transform) override;
	/**
	 * Fills Data from the Details panel and self-initializes GAS when no spawner did it (hand-placed zone).
	 * On the server: starts judging the zone.
	 */
	virtual void BeginPlay() override;
	/** Removes the infinite effects the zone still holds on anyone. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Server. What a zone does beyond applying its effects, for every actor the judge just found inside it. */
	virtual void OnZoneJudged(TArray<FGeoHazardJudge::FTargetResult> const& Results) {}

	/** The radius the zone judges and draws with: Data.Params.Size (the one both the placed and the spawned path end up
	 * in, never written after spawn) grown toward Data.Params.GrowToSize by GrowthAlpha. */
	float GetRadius() const;

	/** Matches the capsule and ZoneVisual to GetRadius(). */
	void ApplyRadius() const;

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	FDeployableData Data;

	/** Server time of the server's BeginPlay, the growth's start. */
	UPROPERTY(Replicated)
	float GrowthStartServerTime = 0.f;

	/** The zone's circle on the zone layer, Ground. */
	UPROPERTY(VisibleAnywhere, Category = "GeoEffectZone")
	TObjectPtr<UGeoZoneVisualComponent> ZoneVisual;

private:
	// Hand-placed zones only — a spawned zone reads all of these off the FDeployableData its ability filled in.
	/** Effects applied to every matching actor: the per-second ones tick, the others persist while inside. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone")
	TArray<TInstancedStruct<FEffectData>> EffectDataArray;
	/** Team this zone belongs to; drives the attitude check against overlapping actors. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone")
	ETeam Team = ETeam::Neutral;
	/** RadiusOverride of the zone in world units. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone", meta = (ClampMin = "0.0"))
	float Radius = 200.f;
	/** Effect level used when applying the effects. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone", meta = (ClampMin = "1"))
	int32 Level = 1;
	/** Colour this zone draws in: a palette slot (EGeoColor::Override has none to draw), Neutral until set. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone")
	FGeoColorParam Color;
	/** Further colours this zone draws beside Color, one per extra meaning it carries. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone")
	TArray<FGeoColorParam> SecondaryColors;
	/** Which attitudes (relative to the zone's team) receive the effects. */
	UPROPERTY(EditAnywhere, Category = "GeoEffectZone",
			  meta = (Bitmask, BitmaskEnum = "/Script/GeoTrinity.ETeamAttitudeBitflag"))
	int32 AttitudeBitmask = TeamAttitudeMask::All;

	/** Server. Judges ZoneJudge, ending it once the zone blinks or expires, then runs again next tick until it is
	 * over. A timer rather than Tick, which Expire turns off before the judge is done. */
	void JudgeZone();

	FGeoHazardJudge ZoneJudge;

	/** 0 at Data.Params.Size, 1 at Data.Params.GrowToSize. */
	float GrowthAlpha = 0.f;

	/** Client-side sizing and tinting: a spawned zone's Data only arrives after the actor exists. */
	UFUNCTION()
	void OnRep_Data();

	/** Gives ZoneVisual Data.Params.Color and its secondary colours. */
	void ApplyColor() const;
};
