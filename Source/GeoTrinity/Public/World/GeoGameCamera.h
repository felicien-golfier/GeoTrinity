// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"

#include "GeoGameCamera.generated.h"

class UCurveVector;

/**
 * Arena-bounded follow camera for GeoTrinity.
 * Follows the centroid of all player pawns in XY, stopping at configurable arena bounds.
 * When the centroid moves back inside the reachable area, the camera catches up smoothly.
 * Follow speed is driven by a UCurveVector sampled at the character's normalized position in the arena
 * (-1 = BoundsMin, 0 = center, +1 = BoundsMax). X channel = X-axis speed (cm/s), Y channel = Y-axis speed (cm/s).
 * The curve should reach 0 at ±1 so the camera stops smoothly at the bounds.
 * Z position is fixed to the actor's initial spawn height.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API AGeoGameCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AGeoGameCamera();

	virtual void Tick(float DeltaTime) override;

protected:
	/** Minimum XY corner of the arena bounds. Camera will not move beyond this point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Bounds")
	FVector2D BoundsMin = FVector2D(-500.f, -500.f);

	/** Maximum XY corner of the arena bounds. Camera will not move beyond this point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Bounds")
	FVector2D BoundsMax = FVector2D(500.f, 500.f);

	/**
	 * Controls camera follow speed per axis based on the character's normalized position within the arena.
	 * Curve X axis: normalized character position, -1 = BoundsMin, 0 = center, +1 = BoundsMax.
	 * X channel: camera speed along the X world axis (cm/s).
	 * Y channel: camera speed along the Y world axis (cm/s).
	 *
	 * Design intent: high speed near 0 (full follow at center), falling to 0 at ±1 (camera stops at bounds).
	 * The symmetric falloff makes the camera decelerate as the character approaches a bound,
	 * and accelerate again as the character moves back toward the center.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement")
	TObjectPtr<UCurveVector> CatchUpCurve;
};
