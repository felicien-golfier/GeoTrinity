// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/SafeSpot/SafeSpot.h"
#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"
#include "Tool/Visual/GeoColor.h"
#include "World/ZoneLayer/GeoZoneShape.h"

class UMaterialParameterCollection;
class UNiagaraComponent;
class UNiagaraSystem;
class UGeoZoneVisualComponent;

#include "DevastatingWavePattern.generated.h"

/**
 * Expanding radial wave fired from the boss's center position.
 * Hits interactable actors only while the wave front passes over them (center within AnnulusWidth of the edge).
 * Pillars are recalled (triggering their explosion); all other hostiles receive WaveEffectDataArray.
 * Runs identically on all clients via PatternStartMulticast. Damage is server-only; pillar detection runs on
 * every machine (deterministic from server time + replicated pillars) to drive the masked AOE VFX.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UDevastatingWavePattern : public UPattern
{
	GENERATED_BODY()

public:
	/** Turns the hazard on. */
	UDevastatingWavePattern();
	/** Spawns the masked AOE Niagara component deactivated and the hidden telegraph circle on the zone layer — the
	 * pattern instance is reused across activations. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Clears the wave's pillar spots and resets all MPC mask slots to the unused sentinel.
	 * Called at the start of both InitPattern and StartPattern so stale data from a previous activation never bleeds
	 * in. */
	void ClearData();

protected:
	/** Clears previous run data, teleports the instigator to the wave origin, then uploads the pillar shadows and shows
	 * the telegraph circle at MaxRadius, filling from the centre over the wind-up (skipped on the "too late" path when
	 * TravelTime >= StartDelay). */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Adds the pillars within MaxDistance of the wave origin (0 = all) that are not spots yet, and uploads the spots
	 * to the VFX mask. Called with all pillars in the wind-up so safe zones appear on the static telegraph. */
	void AddPillarSpots(float MaxDistance);
	/** Resets wave tracking data and MPC pillar slots via ClearData(), hides the telegraph, then activates the real
	 * expanding-wave AOE. */
	virtual void StartPattern() override;
	/** Sets the cue source location to the boss's 2D wave origin. */
	virtual FGameplayCueParameters FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload) override;
	/** Adds the pillars the wave front has reached to the VFX mask, on every machine. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** The wave is live until its front reaches MaxRadius. */
	virtual float GetHazardDuration() const override;
	/** True when Location sits within AnnulusWidth behind the wave front and no pillar shadows it: entering the band
	 * costs a hit, staying in it costs nothing more, stepping back into it costs another one. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** Lets the AOE VFX play out its fade. */
	virtual void OnHazardEnd() override;
	/** Ends the wave: hides the telegraph, the server kills the pillars it reached; a force-stop also removes the AOE
	 * VFX at once. */
	virtual void EndPattern(bool bForceStop = false) override;

private:
	/** Positions the AOE component at the wave origin, pushes its user params and activates it. */
	void ActivateAOEVfx() const;
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

	/** Masked AOE system (NS_PillarsAOE) grown alongside the wave on every rendering machine. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoDevastatingWave|VFX")
	TObjectPtr<UNiagaraSystem> AOEVfxSystem;

	/** MPC_MaskedArea — receives pillar world positions to cut safe zones out of the AOE material. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoDevastatingWave|VFX")
	TObjectPtr<UMaterialParameterCollection> MaskMaterialParameterCollection;

	UPROPERTY(EditDefaultsOnly, Category = "GeoDevastatingWave|VFX")
	FGeoColorParam AOEColor{FLinearColor::Yellow};

	UPROPERTY(EditDefaultsOnly, Category = "GeoDevastatingWave|VFX")
	float FadeOutDuration = 0.5f;

	/** Where the wind-up telegraph's fill grows from. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoDevastatingWave|VFX")
	EGeoFillMode TelegraphFillMode = EGeoFillMode::FromCore;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> AOEVfxComponent;

	/** The wind-up telegraph on the zone layer: the full MaxRadius circle, pillar shadows cut; null on a dedicated
	 * server. */
	UPROPERTY(Transient)
	TObjectPtr<UGeoZoneVisualComponent> TelegraphVisual;

	/** The pillars the wave has reached, as spots (Source = the pillar): the hit test, the mask and the end-of-wave
	 * kill read this one list. */
	TArray<FSafeSpot> PillarSpots;
};
