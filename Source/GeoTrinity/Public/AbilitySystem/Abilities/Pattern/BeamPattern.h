// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "CoreMinimal.h"
#include "Tool/GeoColor.h"
#include "Tool/GeoNiagaraParams.h" // FBeamVfxAssetSet, ApplySwappableAsset

#include "BeamPattern.generated.h"

class UGeoAbilitySystemComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Beam fired from the payload origin along the payload yaw, staying on for BeamDuration.
 * SweepAngle turns it into a rotating sweep (0 leaves it pointing straight ahead); every hostile is hit the tick it
 * enters the beam, so crossing it costs one hit no matter how slowly you walk through — and one more per re-entry.
 * Per-second effects are the exception: they keep ticking on everyone standing in the beam.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UBeamPattern : public UTickablePattern
{
	GENERATED_BODY()

protected:
	/** Spawns the deactivated beam Niagara component; the pattern instance and its component are reused per activation.
	 */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;

	/** Reads SweepAngle from PatternData and stores the activation payload for use during the tick sweep. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Swaps the telegraph for the real beam VFX the moment the beam goes live. */
	virtual void StartPattern() override;
	/** Keeps the windup telegraph aimed while the boss moves and turns. */
	virtual void TickDuringInit(float SpentTime) override;
	/** Aims the beam for the elapsed sweep fraction and applies the effect data to every actor entering it this tick. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** Switches the beam VFX off and clears the per-activation hit set. */
	virtual void EndPattern(bool bForceStop = false) override;
	/** Adds the beam length so the telegraph cue can size itself. */
	virtual FGameplayCueParameters FillCueParam(FAbilityPayload const& Payload) override;

	/** Beam yaw at SpentTime: the payload yaw, offset by however much of the sweep arc has been travelled. */
	float GetBeamYaw(float SpentTime) const;

	/** Where the beam starts: the boss's live location with FollowBossLocation, the payload origin otherwise. */
	FVector GetBeamOrigin() const;

	/** Places the beam VFX where GetBeamOrigin/GetBeamYaw put it at SpentTime. */
	void MoveBeamVfx(float SpentTime);

	/** Applies the EffectDataArray entries matching bPerSecond to every actor of Actors, stopping early if one of them
	 * ends the pattern. */
	void ApplyBeamEffects(bool bPerSecond, TArray<AActor*> const& Actors,
						  UGeoAbilitySystemComponent* SourceASC) const;

	/** Full arc swept over BeamDuration, in degrees, centered on the payload yaw. 0 keeps the beam static. */
	float SweepAngle;

	/** How long the beam stays on, in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam", meta = (ClampMin = "0.01"))
	float BeamDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam", meta = (ClampMin = "0.0"))
	float BeamRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam", meta = (ClampMin = "0.0"))
	float BeamHalfWidth = 60.f;

	/** How a target's own collision radius counts toward the beam hit test. Automatic = center-only for hostiles. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam")
	ETargetOverlapMode OverlapMode = ETargetOverlapMode::Automatic;

	// NOT DETERMINISTIC !!
	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam")
	bool FollowBossOrientation = false;

	// NOT DETERMINISTIC !!
	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam")
	bool FollowBossLocation = false;

	/** Beam visual, authored local-space pointing +X like the systems UGeoBeamVFXComponent drives. Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam|GameFeel")
	TObjectPtr<UNiagaraSystem> BeamVfxSystem;

	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam|GameFeel")
	FGeoColorParam BeamColor;

private:
	/** Windup preview asset (Ray Zone Indicator), loaded once from UGameDataSettings::RayIndicatorSystem in OnCreate —
	 * no per-pattern configuration needed. Same project-wide asset UGeoBeamVFXComponent uses; one NiagaraComponent
	 * serves both looks, swapped via GeoNiagaraParams::ApplySwappableAsset. Leaving the settings value unset shows
	 * BeamVfxSystem for the whole windup, as before. */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> IndicatorSystem;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BeamVfxComponent;
};
