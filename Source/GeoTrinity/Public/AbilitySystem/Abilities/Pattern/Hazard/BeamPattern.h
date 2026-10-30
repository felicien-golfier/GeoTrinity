// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Pattern.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "CoreMinimal.h"
#include "Tool/Visual/GeoColor.h"

#include "BeamPattern.generated.h"

class UGeoZoneVisualComponent;

/**
 * Beam fired from the payload origin along the payload yaw, staying on for BeamDuration.
 * SweepAngle turns it into a rotating sweep (0 leaves it pointing straight ahead); every hostile is hit the tick it
 * enters the beam, so crossing it costs one hit no matter how slowly you walk through — and one more per re-entry.
 * Per-second effects are the exception: they keep ticking on everyone standing in the beam.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UBeamPattern : public UPattern
{
	GENERATED_BODY()

public:
	/** Turns the hazard on. */
	UBeamPattern();

protected:
	/** Creates the hidden beam shape on rendering machines; the pattern instance and its shape are reused per
	 * activation. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;

	/** Stores the activation payload for use during the tick sweep and shows the beam as a telegraph, filling over the
	 * wind-up. */
	virtual void InitPattern(FAbilityPayload const& Payload,
							 TInstancedStruct<FPatternData> const& PatternData) override;
	/** Turns the telegraph into the live beam, on the ground layer, filling over the hazard's duration. */
	virtual void StartPattern() override;
	/** Keeps the windup telegraph aimed while the boss moves and turns. */
	virtual void TickDuringInit(float SpentTime) override;
	/** Aims the beam for the elapsed sweep fraction; the server also draws the hit rectangle when debugging. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** The beam is live for BeamDuration. */
	virtual float GetHazardDuration() const override;
	/** True when Target, at Location, overlaps the beam rectangle as aimed at SpentTime. A beam following the boss
	 * follows it as Target's screen showed it then — GeoLib::GetReplicationDelay earlier, the boss reaching it through
	 * replication. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** Hides the beam. */
	virtual void OnHazardEnd() override;
	/** Hides the beam, still up when force-stopped. */
	virtual void EndPattern(bool bForceStop = false) override;
	/** Adds the beam length so the telegraph cue can size itself. */
	virtual FGameplayCueParameters FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload) override;

	/** Beam yaw at SpentTime: the payload yaw, offset by however much of the sweep arc has been travelled; with
	 * FollowBossOrientation, the boss's yaw at BossServerTime. */
	float GetBeamYaw(float SpentTime, float BossServerTime) const;

	/** Where the beam starts: the boss's location at BossServerTime with FollowBossLocation, the payload origin
	 * otherwise. The current server time follows the boss this machine shows. */
	FVector GetBeamOrigin(float BossServerTime) const;

	/** Places the beam where GetBeamOrigin/GetBeamYaw put it at SpentTime. Ticked in the wind-up (negative SpentTime)
	 * too. */
	virtual void MoveBeamVfx(float SpentTime);

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

	/** Colour of the beam and of its windup telegraph, and of their outlines. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam|GameFeel")
	FGeoColorParam BeamColor;

	/** Further colours the beam and its telegraph draw beside BeamColor, one per extra meaning the beam carries, mixed
	 * by the zone layer. */
	UPROPERTY(EditDefaultsOnly, Category = "GeoBeam|GameFeel")
	TArray<FGeoColorParam> SecondaryBeamColors;

	/** The beam on the zone layer, placed in world space by MoveBeamVfx; null on a dedicated server. */
	UPROPERTY(Transient)
	TObjectPtr<UGeoZoneVisualComponent> BeamVisual;
};
