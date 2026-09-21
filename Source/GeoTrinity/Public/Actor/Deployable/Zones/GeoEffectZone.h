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
 * The server judges the zone as a hazard (FGeoHazardJudge), each actor where it stood when its screen showed it: the
 * per-second entries for the time spent inside, any other entry on entering, its infinite ones removed on leaving.
 * The zone stops acting once it blinks or expires — a placed one never does.
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
	 * On the server: starts judging the zone.
	 */
	virtual void BeginPlay() override;
	/** Removes the infinite effects the zone still holds on anyone. */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(ReplicatedUsing = OnRep_Data)
	FDeployableData Data;

private:
	// Hand-placed zones only — a spawned zone reads all six off the FDeployableData its ability filled in.
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

	/** Server. Judges ZoneJudge, ending it once the zone blinks or expires, then runs again next tick until it is
	 * over. A timer rather than Tick, which Expire turns off before the judge is done. */
	void JudgeZone();

	FGeoHazardJudge ZoneJudge;

	/** Client-side sizing and tinting: a spawned zone's Data only arrives after the actor exists. */
	UFUNCTION()
	void OnRep_Data();

	/** Matches the capsule to Data.Params.Size — the one radius both the placed and the spawned path end up in. */
	void ApplyRadius() const;
	/** Writes Data.Params.Color into ColorParameterNames on every mesh of the zone. */
	void ApplyColor() const;
};
