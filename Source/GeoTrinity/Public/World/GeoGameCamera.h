// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"

#include "GeoGameCamera.generated.h"

class UCurveLinearColor;

/**
 * Arena-bounded follow camera for GeoTrinity.
 * Follows the centroid of all player pawns in XY, stopping at configurable arena bounds.
 * When the centroid moves back inside the reachable area, the camera catches up smoothly.
 * Catch-up speed is driven by a UCurveLinearColor: curve X = distance to target (cm),
 * R channel = X-axis speed (cm/s), G channel = Y-axis speed (cm/s).
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
	 * Controls catch-up speed per axis as a function of distance to the target position.
	 * Curve X axis: distance to target in cm.
	 * R channel: camera speed along the X world axis (cm/s).
	 * G channel: camera speed along the Y world axis (cm/s).
	 *
	 * @note A curve that ramps from low speed at close range to high speed at long range
	 *       gives a smooth, responsive catch-up feel without overshooting.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement")
	TObjectPtr<UCurveLinearColor> CatchUpCurve;
};
