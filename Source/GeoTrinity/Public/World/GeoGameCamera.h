// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"

#include "GeoGameCamera.generated.h"

class AGeoCharacter;
class AGeoCameraVolume;

/**
 * Orthographic follow camera for GeoTrinity.
 * Always follows the local player with exponential smoothing — no edge-trigger dead zone.
 * Its movement bounds are the `TargetPoint.CameraBounds` corner points of whichever `AGeoCameraVolume` the local
 * player currently stands in: framing is a pure function of location, unrelated to the arena or the match state.
 * Inside a volume the follow target is clamped to those bounds; outside every volume the camera follows freely
 * (hub, corridors, a post-wipe teleport to the entrance). Bounds are recomputed only when the active volume changes.
 * Near a border the clamped target shrinks the interp gap, so the camera decelerates naturally.
 * Z position is fixed to the actor's initial spawn height.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API AGeoGameCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AGeoGameCamera();

	/** Follows the local player with exponential smoothing; clamps to the active volume's bounds; pans freely when spectating. */
	virtual void Tick(float DeltaTime) override;

	/** Called by an AGeoCameraVolume when the local player enters it; the most recently entered volume frames the camera. */
	void EnterVolume(AGeoCameraVolume* Volume);
	/** Called by an AGeoCameraVolume when the local player leaves it. */
	void ExitVolume(AGeoCameraVolume* Volume);

protected:
	/** Exponential follow speed. Higher = snappier. Typical range 2–8. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement", meta = (ClampMin = "0.1"))
	float FollowInterpSpeed = 5.f;

	/** Free-camera pan speed (units/s) while spectating (local player dead). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement", meta = (ClampMin = "0.0"))
	float SpectateMoveSpeed = 1500.f;

private:
	/** The volume framing the camera: the most recently entered one still overlapping the player, or null when in none. */
	AGeoCameraVolume* GetActiveVolume();
	/** Recomputes `Bounds` from the active volume's `TargetPoint.CameraBounds` points; clears `bBounded` when in none. */
	void RefreshBounds();

	/** Reads the move-action value straight from the Enhanced Input player subsystem — the dead pawn's input
	 * component is disabled, so the pawn's own move callback never fires while spectating. */
	FVector2D GetSpectateMoveInput(APlayerController const* PlayerController,
								   AGeoCharacter const* LocalCharacter) const;

	/** Volumes the local player is currently inside, in entry order; the last is the one that frames the camera. */
	TArray<TWeakObjectPtr<AGeoCameraVolume>> ActiveVolumes;
	/** World-space XY bounds the follow target is clamped to while `bBounded`; recomputed on volume change. */
	FBox2D Bounds{};
	/** True while inside a volume that resolved to at least one camera-bounds point. */
	bool bBounded = false;
	/** Free-camera target while the local player is dead; driven by move input, clamped to `Bounds` while bounded. */
	FVector2D SpectateTarget = FVector2D::ZeroVector;
	bool bSpectating = false;
};
