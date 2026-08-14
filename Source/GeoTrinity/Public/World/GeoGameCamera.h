// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"

#include "GeoGameCamera.generated.h"

class AGeoCharacter;
class AGeoCameraVolume;
class UMaterialInterface;
class UMaterialParameterCollection;
class UGeoBackgroundPulseComponent;
class UGeoBackdropComponent;

/**
 * Orthographic follow camera for GeoTrinity.
 * Always follows the living local players with exponential smoothing — no edge-trigger dead zone. In couch coop it
 * frames their midpoint and widens `OrthoWidth` as they spread away from the camera; there is never a split view.
 * Its movement bounds are the `TargetPoint.CameraBounds` corner points of whichever `AGeoCameraVolume` a local
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
	/** Configures the camera component for orthographic projection and initialises movement defaults. */
	AGeoGameCamera();

	/** Caches the authored OrthoWidth as the zoomed-in baseline every zoom-out starts from, and installs the outline
	 * post-process. */
	virtual void BeginPlay() override;

	/** Follows the living local players with exponential smoothing; clamps to the active volume's bounds; once every
	 * local player is dead, holds at the death position for `AGeoGameState::DeathTime` before accepting pan input. */
	virtual void Tick(float DeltaTime) override;

	/** Called by an AGeoCameraVolume when a local player enters it; the most recently entered volume frames the camera.
	 */
	void EnterVolume(AGeoCameraVolume* Volume);
	/** Called by an AGeoCameraVolume when a local player leaves it. */
	void ExitVolume(AGeoCameraVolume* Volume);

protected:
	/** Exponential follow speed. Higher = snappier. Typical range 2–8. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCamera|Movement", meta = (ClampMin = "0.1"))
	float FollowInterpSpeed = 5.f;

	/** Free-camera pan speed (units/s) while spectating (every local player dead). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCamera|Movement", meta = (ClampMin = "0.0"))
	float SpectateMoveSpeed = 1500.f;

	/** Distance from the camera to the farthest local player the camera stays fully zoomed in below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCamera|Zoom", meta = (ClampMin = "0.0"))
	float ZoomMinDistance = 600.f;

	/** Distance to the farthest local player at which the camera reaches `MaxOrthoWidth`; past it, it stops widening.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCamera|Zoom", meta = (ClampMin = "0.0"))
	float ZoomMaxDistance = 1500.f;

	/** OrthoWidth reached at `ZoomMaxDistance`; past it players simply leave the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCamera|Zoom", meta = (ClampMin = "0.0"))
	float MaxOrthoWidth = 5000.f;

	/** Exponential zoom speed. Deliberately slower than FollowInterpSpeed so framing doesn't pump. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCamera|Zoom", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 3.f;

	/** Deployable outline post-process (MI_DeployableOutline). Installed as a blendable in BeginPlay rather than
	 * authored on the camera component, because it only works once fed the palette texture. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoCamera|Outline")
	TObjectPtr<UMaterialInterface> OutlineMaterial;

	/** MPC_Camera: the view state the parallax backdrop layers need, since orthographic projection gives them no
	 * parallax of their own. Republished every tick. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoCamera|Backdrop")
	TObjectPtr<UMaterialParameterCollection> CameraParameters;

	/** Drives the background lattice's pulse slots. A component rather than a spawned actor so it rides the camera
	 * for free and every knob is authorable here on BP_GeoCam. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCamera|Backdrop")
	TObjectPtr<UGeoBackgroundPulseComponent> BackgroundPulse;

	/** Builds the parallax star planes. On the camera because they have to ride the view — orthographic projection
	 * gives a level-placed backdrop no parallax at all. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GeoCamera|Backdrop")
	TObjectPtr<UGeoBackdropComponent> Backdrop;

private:
	/** Installs OutlineMaterial as a post-process blendable, with ColorPalette baked into the lookup texture it
	 * indexes with its custom-depth stencil value. */
	void ApplyOutlineMaterial();

	/** Writes this frame's view state to CameraParameters: the follow position the backdrop layers offset their UVs
	 * against, and BaseOrthoWidth/CurrentOrthoWidth, which the far layers scale by to stay the same size on screen
	 * while the camera zooms out. */
	void PublishCameraParameters(FVector2D CameraXY);

	/** The volume framing the camera: the most recently entered one still overlapping the player, or null when in none.
	 */
	AGeoCameraVolume* GetActiveVolume();
	/** Recomputes `Bounds` from the active volume's `TargetPoint.CameraBounds` points; clears `bBounded` when in none.
	 */
	void RefreshBounds();

	/** Reads the move-action value straight from the Enhanced Input player subsystem — the dead pawn's input
	 * component is disabled, so the pawn's own move callback never fires while spectating. */
	FVector2D GetSpectateMoveInput(APlayerController const* PlayerController,
								   AGeoCharacter const* LocalCharacter) const;

	/** Volumes local players are currently inside; one entry per player per volume, so a volume stays active while
	 * any of them remains inside. The last valid entry is the one that frames the camera. */
	TArray<TWeakObjectPtr<AGeoCameraVolume>> ActiveVolumes;
	/** World-space XY bounds the follow target is clamped to while `bBounded`; recomputed on volume change. */
	FBox2D Bounds{};
	/** True while inside a volume that resolved to at least one camera-bounds point. */
	bool bBounded = false;
	/** Free-camera target while every local player is dead; driven by move input, clamped to `Bounds` while bounded. */
	FVector2D SpectateTarget = FVector2D::ZeroVector;
	bool bSpectating = false;
	/** Counts down from `AGeoGameState::DeathTime` (captured on death) before panning input is accepted; the camera
	 * holds still at its death position until it reaches zero. */
	float SpectateDelayRemaining = 0.f;
	/** Authored OrthoWidth, captured in BeginPlay: the zoomed-in framing zoom-to-fit never goes below. */
	float BaseOrthoWidth = 0.f;
	float CurrentOrthoWidth = 0.f;
};
