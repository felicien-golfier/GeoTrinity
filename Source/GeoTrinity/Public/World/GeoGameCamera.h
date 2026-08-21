// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"

#include "GeoGameCamera.generated.h"

class AGeoCameraVolume;
class UGeoInputComponent;
class UInputAction;
class UMaterialInterface;
class UMaterialParameterCollection;
class UGeoBackgroundPulseComponent;
class UGeoBackdropComponent;
struct FInputActionValue;

/**
 * Orthographic follow camera for GeoTrinity.
 * Always follows the living local players with exponential smoothing — no edge-trigger dead zone. In couch coop it
 * frames their midpoint and widens `OrthoWidth` as they spread away from the camera; there is never a split view.
 * Whichever `AGeoCameraVolume` a local player currently stands in frames it: the follow target is clamped to that
 * volume's bounds box. Framing is a pure function of location, unrelated to the arena or the match state — outside
 * every volume the camera follows freely (hub, corridors, a post-wipe teleport to the entrance).
 * Zoom is one value, `TargetZoom`: a volume sets it on entry and the mouse wheel nudges it from wherever it is, both
 * writing it directly. `Tick` only ever interpolates towards it and holds it inside the Game Data Settings'
 * Min/MaxOrthoWidth. It is the *closest* the camera ever sits — the coop spread only ever widens past it.
 * Near a border the clamped target shrinks the interp gap, so the camera decelerates naturally.
 * Z position is fixed to the actor's initial spawn height.
 */
UCLASS(BlueprintType, Blueprintable)
class GEOTRINITY_API AGeoGameCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	/** Enables tick and creates the BackgroundPulse and Backdrop native subobjects. */
	AGeoGameCamera();

	/** Caches the authored OrthoWidth as the zoom the camera starts on, and installs the outline post-process. */
	virtual void BeginPlay() override;

	/** Follows the living local players with exponential smoothing; clamps to the active volume's bounds; once every
	 * local player is dead, holds at the death position for `AGeoGameState::DeathTime` before accepting pan input. */
	virtual void Tick(float DeltaTime) override;

	/** Called by an AGeoCameraVolume when a local player enters it; the most recently entered volume frames the camera.
	 */
	void EnterVolume(AGeoCameraVolume* Volume);
	/** Called by an AGeoCameraVolume when a local player leaves it. */
	void ExitVolume(AGeoCameraVolume* Volume);

	/** Moves `TargetZoom` by one wheel notch, positive zooming in. Called from UGeoInputComponent's zoom binding; the
	 * next tick is what holds the result inside the settings' range. */
	void AddZoomInput(float WheelValue);

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

	/** Distance to the farthest local player at which the camera reaches the settings' MaxOrthoWidth; past it, it stops
	 * widening. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCamera|Zoom", meta = (ClampMin = "0.0"))
	float ZoomMaxDistance = 1500.f;

	/** OrthoWidth one mouse-wheel notch adds or removes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoCamera|Zoom", meta = (ClampMin = "0.0"))
	float ZoomWheelStep = 300.f;

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

	/** Current value of Action, read straight from the Enhanced Input player subsystem — the dead pawn's input
	 * component is disabled, so the pawn's own callbacks never fire while spectating. Zero when unbound. */
	FInputActionValue ReadInputValue(APlayerController const* PlayerController, UInputAction const* Action) const;

	/** Zooms to the active volume's OrthoWidth. Only entering a volume calls it — leaving one keeps the zoom it was
	 * on, wheel nudges included, until another volume takes the framing over. */
	void RefreshVolumeZoom();

	/** Collects the XY of every local player the camera has to frame, plus the first local controller and its input
	 * component (null when its pawn is not an AGeoCharacter), which the spectate pan reads its input from. */
	void GatherLocalPlayers(TArray<FVector2D, TInlineAllocator<4>>& OutLivingPlayers,
							APlayerController const*& OutFirstController,
							UGeoInputComponent const*& OutFirstInputComponent) const;

	/** Enters and leaves spectate mode, freezing `SpectateTarget` on the death transition and advancing the pan once
	 * `SpectateDelayRemaining` has run out. */
	void UpdateSpectateState(bool bAllLocalPlayerDead, FVector2D CameraXY,
							 APlayerController const* FirstLocalController,
							 UGeoInputComponent const* FirstLocalInputComponent, float DeltaTime);

	/** Holds `TargetZoom` inside the settings' zoom range, then interpolates `CurrentOrthoWidth` towards it, widened
	 * until the farthest local player fits. */
	void UpdateZoom(TConstArrayView<FVector2D> LivingPlayers, FVector2D CameraXY, float DeltaTime);

	/** Geo.ShowCameraZoom: one line per living local player plus the distances the zoom was derived from. */
	void DrawZoomDebug(TConstArrayView<FVector2D> LivingPlayers, FVector2D CameraXY, float FarthestDistance) const;

	/** Pulls the follow target in by the on-screen half-extents of the current OrthoWidth at the camera component's
	 * yaw, so the view never shows past the bounds box; snaps to its centre on an axis too small to fit the view.
	 * Identity outside every volume, and inside one that doesn't limit the view. Half-height comes from the camera's
	 * own AspectRatio, which is what UE projects with while bConstrainAspectRatio holds — the window's shape never
	 * reaches the view, so it must not reach the clamp either. */
	FVector2D ClampToVolume(FVector2D Target, AGeoCameraVolume const* Volume) const;

	/** Volumes local players are currently inside; one entry per player per volume, so a volume stays active while
	 * any of them remains inside. The last valid entry is the one that frames the camera. */
	TArray<TWeakObjectPtr<AGeoCameraVolume>> ActiveVolumes;
	/** Free-camera target while every local player is dead; driven by move input, clamped to the active volume. */
	FVector2D SpectateTarget = FVector2D::ZeroVector;
	bool bSpectating = false;
	/** Counts down from `AGeoGameState::DeathTime` (captured on death) before panning input is accepted; the camera
	 * holds still at its death position until it reaches zero. */
	float SpectateDelayRemaining = 0.f;
	/** Authored OrthoWidth, captured in BeginPlay: the zoom the camera starts on, and the baseline the backdrop layers
	 * scale against. */
	float BaseOrthoWidth = 0.f;
	/** Width the camera zooms to — written by a volume on entry and by the wheel, held inside the settings' zoom range
	 * by UpdateZoom. The couch-coop spread widens past it but never below it. */
	float TargetZoom = 0.f;
	float CurrentOrthoWidth = 0.f;
};
