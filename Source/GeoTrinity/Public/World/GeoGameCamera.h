// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"

#include "GeoGameCamera.generated.h"

class UCurveVector;

/**
 * Orthographic follow camera for GeoTrinity.
 * Stays stationary while the player centroid is well within the visible area.
 * Movement is only triggered when the centroid enters the last ScreenEdgeThresholdPercent of the screen.
 * Follow speed is driven by FollowSpeedCurve, sampled at edge proximity [0 = just entered trigger zone, 1 = at screen edge].
 * Camera stops moving when the map boundary (BoundsMin/BoundsMax) would become visible within the viewport.
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
	/** Minimum XY corner of the map bounds. Camera stops when this boundary would become visible inside the viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Bounds")
	FVector2D BoundsMin = FVector2D(-500.f, -500.f);

	/** Maximum XY corner of the map bounds. Camera stops when this boundary would become visible inside the viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Bounds")
	FVector2D BoundsMax = FVector2D(500.f, 500.f);

	/**
	 * Fraction of the visible screen half-extent near each edge that triggers camera movement.
	 * 0.05 means the camera starts moving when the character is within the last 5% of the screen.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float ScreenEdgeThresholdPercent = 0.05f;

	/**
	 * Camera follow speed curve. Curve X axis: edge proximity [0 = character just entered trigger zone, 1 = character at screen edge].
	 * X channel: camera speed along the world X axis (cm/s).
	 * Y channel: camera speed along the world Y axis (cm/s).
	 *
	 * Design intent: 0 speed at proximity 0 (movement just triggered), ramping up to maximum speed at proximity 1 (character at screen edge).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement")
	TObjectPtr<UCurveVector> FollowSpeedCurve;
};
