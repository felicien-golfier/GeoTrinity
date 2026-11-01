// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/Pattern/Hazard/ZoneHazardPattern.h"
#include "AbilitySystem/Data/GeoAbilityTargetTypes.h"
#include "CoreMinimal.h"

#include "BeamPattern.generated.h"

/**
 * Beam fired from the payload origin along the payload yaw, staying on for BeamDuration.
 * SweepAngle turns it into a rotating sweep (0 leaves it pointing straight ahead); every hostile is hit the tick it
 * enters the beam, so crossing it costs one hit no matter how slowly you walk through — and one more per re-entry.
 * Per-second effects are the exception: they keep ticking on everyone standing in the beam.
 */
UCLASS(Blueprintable)
class GEOTRINITY_API UBeamPattern : public UZoneHazardPattern
{
	GENERATED_BODY()

protected:
	/** Reads the sweep angle of the launching ability. */
	virtual void OnCreate(FGameplayTag AbilityTag, AActor& Owner) override;

	/** Aims the beam for the elapsed sweep fraction; the server also draws the hit rectangle when debugging. */
	virtual void TickPattern(float ServerTime, float SpentTime) override;
	/** The beam is live for BeamDuration. */
	virtual float GetHazardDuration() const override;
	/** True when Target, at Location, overlaps the beam rectangle as aimed at SpentTime. A beam following the boss
	 * follows it as Target's screen showed it then — GeoLib::GetReplicationDelay earlier, the boss reaching it through
	 * replication. */
	virtual bool IsInHazard(AActor const* Target, FVector2D Location, float SpentTime) const override;
	/** Adds the beam length so the telegraph cue can size itself. */
	virtual FGameplayCueParameters FillCueParam(FGeoCueParam const& Cue, FAbilityPayload const& Payload) override;

	/** Beam yaw at SpentTime: the payload yaw, offset by however much of the sweep arc has been travelled; with
	 * FollowBossOrientation, the boss's yaw at BossServerTime. */
	float GetBeamYaw(float SpentTime, float BossServerTime) const;

	/** Where the beam starts: the boss's location at BossServerTime with FollowBossLocation, the payload origin
	 * otherwise. The current server time follows the boss this machine shows. */
	FVector GetBeamOrigin(float BossServerTime) const;

	/** A BeamRange x 2 BeamHalfWidth beam, placed where GetBeamOrigin/GetBeamYaw put it at SpentTime. */
	virtual void MoveHazardVisual(float SpentTime) override;

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
};
