// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"

#include "GeoGameCamera.generated.h"

/**
 * Orthographic follow camera for GeoTrinity.
 * Always follows the local player with exponential smoothing — no edge-trigger dead zone.
 * World-space bounds are computed at BeginPlay from AGeoTargetPoint actors tagged Camera.Bounds.
 * Camera decelerates naturally at borders because the clamped target shrinks the interp gap.
 * Z position is fixed to the actor's initial spawn height.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API AGeoGameCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AGeoGameCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	/** Exponential follow speed. Higher = snappier. Typical range 2–8. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement", meta = (ClampMin = "0.1"))
	float FollowInterpSpeed = 5.f;

private:
	/** Computes the world-space AABB (Min in X/Y, Max in Z/W) from the AGeoTargetPoints tagged for the current match
	 * state. Falls back to ±500 when no points are found. */
	UFUNCTION()
	void CalculateBounds();

	FBox2D Bounds{};
};
