// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"
#include "AbilitySystem/Abilities/Pattern/Hazard/ZoneHazardPattern.h"
#include "CoreMinimal.h"

#include "DevastatingWavePattern.generated.h"

/**
 * Expanding radial wave fired from the boss's center position.
 * Hits interactable actors only while the wave front passes over them (center within AnnulusWidth of the edge).
 * Pillars are recalled (triggering their explosion); all other hostiles receive WaveEffectDataArray.
 * Runs identically on all clients via PatternStartMulticast. Damage is server-only; pillar detection runs on
 * every machine (deterministic from server time + replicated pillars) to cut their shadows out of the drawn wave: the
 * whole MaxRadius circle in the wind-up, the hit band as a ring once live.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UDevastatingWavePattern : public UZoneHazardPattern
{
	GENERATED_BODY()

public:
	/** Lethal colour. */
	UDevastatingWavePattern();
	/** Clears the wave's pillar spots and frees the mask. Called before InitPattern's telegraph and at StartPattern so
	 * stale spots from a previous activation, or from the wind-up, never bleed in. */
	void ClearData();

protected:
	/** Clears previous run data before the base shows the telegraph, then teleports the instigator to the wave
	 * origin. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Adds the pillars within MaxDistance of the wave origin (0 = all) that are not spots yet, and uploads the spots
	 * to the mask. Called with all pillars in the wind-up so safe zones appear on the static telegraph. */
	void AddPillarSpots(float MaxDistance);
	/** Restarts the pillar spots (ClearData) before the base turns the telegraph into the live ring. */
	virtual void StartPattern() override;
	/** Sets the cue source location to the boss's 2D wave origin. */
	virtual FGameplayCueParameters FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload) override;
	/** Grows the ring and adds the pillars the wave front has reached to the mask, on every machine. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** The wave is live until its front reaches MaxRadius. */
	virtual float GetHazardDuration() const override;
	/** True when Location sits within AnnulusWidth behind the wave front and no pillar shadows it: entering the band
	 * costs a hit, staying in it costs nothing more, stepping back into it costs another one. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** Ends the wave: the server kills the pillars it reached (not on a force-stop). */
	virtual void EndPattern(bool bForceStop = false) override;
	/** Centred on GetCenter(): wind-up = the MaxRadius circle, with every pillar's shadow (AddPillarSpots(0));
	 * live = the ring between the front and AnnulusWidth behind it, the hit band. */
	virtual void MoveHazardVisual(float SpentTime) override;

private:
	/** The wave's centre: the first target location (the ability's TargetPoint). */
	FVector2D GetCenter() const { return GetTargetLocations()[0]; }

	/** Draws the wave front (red) and inner annulus (green) circles, plus each pillar's safe-zone shadow.
	 * Gated on the Geo.DrawDevastatingWave CVar. */
	void DrawDebugWave(float CurrentRadius) const;

	UPROPERTY(EditDefaultsOnly, Category = "GeoDevastatingWave")
	float ExpansionSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "GeoDevastatingWave")
	float MaxRadius = 3000.f;

	/** Width of the damaging band just inside the wave front. Actors are hit only while their center sits between
	 * CurrentRadius and CurrentRadius - AnnulusWidth; once the front passes beyond that band they are safe. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoDevastatingWave")
	float AnnulusWidth = 200.f;

	/** The pillars the wave has reached, as spots (Source = the pillar): the hit test, the mask and the end-of-wave
	 * kill read this one list. */
	TArray<FSafeSpot> PillarSpots;
};
