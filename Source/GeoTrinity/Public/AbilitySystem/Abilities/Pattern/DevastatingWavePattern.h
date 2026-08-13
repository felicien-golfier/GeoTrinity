// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"
#include "Tool/GeoColor.h"

class AGeoPillar;
class UMaterialParameterCollection;
class UNiagaraComponent;
class UNiagaraSystem;

#include "DevastatingWavePattern.generated.h"

USTRUCT()
struct FPillarWaveData
{
	GENERATED_BODY()

	FVector2D Location;
	float Radius = 0.f;
	TWeakObjectPtr<AGeoPillar> Pillar;
};

/**
 * Expanding radial wave fired from the boss's center position.
 * Hits interactable actors only while the wave front passes over them (center within AnnulusWidth of the edge).
 * Pillars are recalled (triggering their explosion); all other hostiles receive WaveEffectDataArray.
 * Runs identically on all clients via PatternStartMulticast. Damage is server-only; pillar detection runs on
 * every machine (deterministic from server time + replicated pillars) to drive the masked AOE VFX.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UDevastatingWavePattern : public UTickablePattern
{
	GENERATED_BODY()

public:
	/** Spawns the masked AOE Niagara component deactivated — the pattern instance is reused across activations. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;
	/** Clears wave pillar data and resets all MPC pillar mask slots to the unused sentinel.
	 * Called at the start of both InitPattern and StartPattern so stale data from a previous activation never bleeds in. */
	void ClearData();

protected:
	/** Clears previous run data, teleports the instigator to the wave origin, then activates the telegraph VFX for
	 * the wind-up phase (skipped on the "too late" path when TravelTime >= StartDelay). */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Pre-populates the MPC pillar-mask slots with all pillars currently visible from the wave origin so safe zones
	 * appear on the static telegraph before the wave starts expanding. */
	void AddAllPillarsToVfxMask();
	/** Resets wave tracking data and MPC pillar slots via ClearData(), then activates the real expanding-wave AOE. */
	virtual void StartPattern() override;
	/** Activates the AOE VFX component in telegraph mode: full MaxRadius extent at AOEColor, grow time =
	 * StartDelay - TravelTime (remaining wind-up). Shows the full danger zone before the wave begins. */
	void ActivateAoeVfxTelegraph() const;
	/** Sets the cue source location to the boss's 2D wave origin. */
	virtual FGameplayCueParameters FillCueParam(FAbilityPayload const& Payload) override;
	/**
	 * Expands the wave radius by ExpansionSpeed * SpentTime each tick.
	 * Hits actors whose center sits within AnnulusWidth of the wave front: pillars are added to the VFX mask on all
	 * machines as the front reaches them; other hostiles receive effect data server-side only, the tick they enter
	 * the band (staying in it costs nothing more, stepping back into it costs another hit).
	 * Ends the pattern when MaxRadius is reached.
	 */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** Ends the wave; deactivates the AOE VFX gracefully on natural completion or immediately on force-stop. */
	virtual void EndPattern(bool bForceStop = false) override;

private:
	bool ShouldHitActor(AActor const* Actor) const;
	/** Writes the last added PillarsWaveData entry into the next mask MPC pillar slot. */
	void AddPillarToVfxMask();
	/** Positions the AOE component at the wave origin, pushes its user params and activates it. */
	void ActivateAOEVfx() const;
	/** Draws the wave front (red) and inner annulus (green) circles, plus each pillar's safe-zone shadow.
	 * Gated on the Geo.DrawDevastatingWave CVar. */
	void DrawDebugWave(float CurrentRadius) const;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	float ExpansionSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	float MaxRadius = 3000.f;

	/** Width of the damaging band just inside the wave front. Actors are hit only while their center sits between
	 * CurrentRadius and CurrentRadius - AnnulusWidth; once the front passes beyond that band they are safe. */
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	float AnnulusWidth = 200.f;

	/** Masked AOE system (NS_PillarsAOE) grown alongside the wave on every rendering machine. */
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX")
	TObjectPtr<UNiagaraSystem> AOEVfxSystem;

	/** MPC_MaskedArea — receives pillar world positions to cut safe zones out of the AOE material. */
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX")
	TObjectPtr<UMaterialParameterCollection> MaskMaterialParameterCollection;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX")
	FGeoColorParam AOEColor{FLinearColor::Yellow};

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX")
	float FadeOutDuration = 0.5f;

	/** Duration in seconds for the telegraph VFX to fade once the wave starts expanding (default 0.1 s — near-instant
	 * handoff so the static telegraph and the expanding wave don't visually overlap). */
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX|Telegraph")
	float TelegraphFadeOutDuration = 0.1f;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> AOEVfxComponent;

	FTimerHandle TelegraphBlinkTimerHandle;

	TArray<FPillarWaveData> PillarsWaveData;
};
