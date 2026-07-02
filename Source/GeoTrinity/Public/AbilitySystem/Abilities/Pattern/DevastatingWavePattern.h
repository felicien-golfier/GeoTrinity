// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "CoreMinimal.h"

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
 * Hits each interactable actor exactly once as the wave radius passes through them.
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
	/** Clears hit-actor tracking, wave pillar data, and resets all MPC pillar mask slots to the unused sentinel.
	 * Called at the start of both InitPattern and StartPattern so stale data from a previous activation never bleeds in. */
	void ClearData();

protected:
	/** Clears previous run data, teleports the instigator to the wave origin, then activates the telegraph VFX for
	 * the wind-up phase (skipped on the "too late" path when TravelTime >= StartDelay). */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	void AddAllPillarsToVfxMask();
	/** Resets wave tracking data and MPC pillar slots via ClearData(), then activates the real expanding-wave AOE. */
	virtual void StartPattern() override;
	void ActivateAoeVfxTelegraph() const;
	/** Sets the cue source location to the boss's 2D wave origin. */
	virtual FGameplayCueParameters FillCueParam(FAbilityPayload const& Payload) override;
	/**
	 * Expands the wave radius by ExpansionSpeed * SpentTime each tick.
	 * Hits each actor in range exactly once: pillars are added to the VFX mask on all machines;
	 * other hostiles receive effect data server-side only. Ends the pattern when MaxRadius is reached.
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
#if WITH_EDITOR
	void DrawDebugSafeZones(float CurrentRadius) const;
#endif
	;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	float ExpansionSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave")
	float MaxRadius = 3000.f;

	/** Masked AOE system (NS_PillarsAOE) grown alongside the wave on every rendering machine. */
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX")
	TObjectPtr<UNiagaraSystem> AOEVfxSystem;

	/** MPC_MaskedArea — receives pillar world positions to cut safe zones out of the AOE material. */
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX")
	TObjectPtr<UMaterialParameterCollection> MaskMaterialParameterCollection;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX")
	FLinearColor AOEColor = FLinearColor::Yellow;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX")
	float FadeOutDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX|Telegraph")
	float TelegraphFadeOutDuration = 0.1f;

	/** Color of the full-range blinking telegraph shown during the wind-up, before the wave starts expanding. */
	UPROPERTY(EditDefaultsOnly, Category = "DevastatingWave|VFX|Telegraph")
	FLinearColor TelegraphColor = FLinearColor::Red;


	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> AOEVfxComponent;

	FTimerHandle TelegraphBlinkTimerHandle;

	TSet<TWeakObjectPtr<AActor>> HitActors;
	TArray<FPillarWaveData> PillarsWaveData;
};
