// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraShakeBase.h"
#include "CoreMinimal.h"

#include "GeoCurveCameraShake.generated.h"

class UCurveVector;

/**
 * Camera shake pattern whose whole envelope is one curve, so a shake escalates, holds and dies wherever its keys say
 * rather than at a fixed amplitude between two blends. Ramp the keys up and the shake grows with the moment it is
 * played over.
 * The curve is sampled in seconds from the moment the shake starts — a notify placed on frame N of a montage hands the
 * curve that montage's own timeline from N on — and its last key ends the shake, so the curve's length is the shake's
 * duration and no separate duration is authored.
 * Offsets stay in the screen plane and the rotation is roll only: this game's camera is orthographic down one axis, so
 * an offset along the view axis is invisible and a pitch or yaw would swing the whole arena.
 */
UCLASS(meta = (DisplayName = "Geo Curve Camera Shake Pattern"))
class GEOTRINITY_API UGeoCurveCameraShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()

public:
	/** Forwards to the pattern base, which declares no default constructor. */
	UGeoCurveCameraShakePattern(FObjectInitializer const& ObjectInitializer) : Super(ObjectInitializer) {}

protected:
	/**
	 * Drives the shake, sampled in seconds since it started: X = screen-plane offset amplitude in units, Y =
	 * oscillation frequency in Hz, Z = roll amplitude in degrees. Same channel convention as the hex barrier's tile
	 * shake, with roll added.
	 */
	UPROPERTY(EditAnywhere, Category = "GeoCameraShake")
	TObjectPtr<UCurveVector> ShakeCurve;

private:
	/** Reports the curve's own length as the shake's duration. */
	virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const override;
	/** Rewinds to the curve's start. Flags a pattern authored without a curve. */
	virtual void StartShakePatternImpl(FCameraShakePatternStartParams const& Params) override;
	/** Advances the elapsed time and writes this frame's screen-plane offset and roll, unscaled — the base class
	 * applies the shake scale and the play space. */
	virtual void UpdateShakePatternImpl(FCameraShakePatternUpdateParams const& Params,
										FCameraShakePatternUpdateResult& OutResult) override;
	/** True once the elapsed time has passed the curve's last key. */
	virtual bool IsFinishedImpl() const override;

	/** Time of the curve's last key. Zero without a curve, which ends the shake on its first update. */
	float GetDuration() const;

	float ElapsedTime = 0.f;
};

/**
 * Camera shake driven by a curve. Subclass it as a Blueprint per shake and author that Blueprint's curve.
 * The pattern is fixed here rather than picked per asset because a shake pattern only reaches the instances the camera
 * spawns when it is the default subobject the base class creates under its own name — one assigned to the property
 * afterwards stays on the class defaults and every spawned shake gets none.
 */
UCLASS(Blueprintable, meta = (DisplayName = "Geo Curve Camera Shake"))
class GEOTRINITY_API UGeoCurveCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	/** Makes the root shake pattern a curve pattern, which is the only thing this class exists to do. */
	UGeoCurveCameraShake(FObjectInitializer const& ObjectInitializer)
		: Super(ObjectInitializer.SetDefaultSubobjectClass<UGeoCurveCameraShakePattern>(TEXT("RootShakePattern")))
	{
	}
};
