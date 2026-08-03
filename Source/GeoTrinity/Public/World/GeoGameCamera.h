// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Camera/CameraActor.h"
#include "CoreMinimal.h"

#include "GeoGameCamera.generated.h"

class AGeoCharacter;
class AGeoCameraVolume;
class UMaterialInterface;

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

	/** Follows the living local players with exponential smoothing; clamps to the active volume's bounds; pans freely
	 * when spectating. */
	virtual void Tick(float DeltaTime) override;

	/** Called by an AGeoCameraVolume when a local player enters it; the most recently entered volume frames the camera.
	 */
	void EnterVolume(AGeoCameraVolume* Volume);
	/** Called by an AGeoCameraVolume when a local player leaves it. */
	void ExitVolume(AGeoCameraVolume* Volume);

protected:
	/** Exponential follow speed. Higher = snappier. Typical range 2–8. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement", meta = (ClampMin = "0.1"))
	float FollowInterpSpeed = 5.f;

	/** Free-camera pan speed (units/s) while spectating (every local player dead). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Movement", meta = (ClampMin = "0.0"))
	float SpectateMoveSpeed = 1500.f;

	/** Distance from the camera to the farthest local player the camera stays fully zoomed in below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float ZoomMinDistance = 600.f;

	/** Distance to the farthest local player at which the camera reaches `MaxOrthoWidth`; past it, it stops widening.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float ZoomMaxDistance = 1500.f;

	/** OrthoWidth reached at `ZoomMaxDistance`; past it players simply leave the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float MaxOrthoWidth = 5000.f;

	/** Exponential zoom speed. Deliberately slower than FollowInterpSpeed so framing doesn't pump. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Zoom", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 3.f;

	/** Deployable outline post-process (MI_DeployableOutline). Installed as a blendable in BeginPlay rather than
	 * authored on the camera component, because it only works once fed the palette texture. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Outline")
	TObjectPtr<UMaterialInterface> OutlineMaterial;

private:
	/** Installs OutlineMaterial as a post-process blendable, with ColorPalette baked into the lookup texture it
	 * indexes with its custom-depth stencil value. */
	void ApplyOutlineMaterial();

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
	/** Authored OrthoWidth, captured in BeginPlay: the zoomed-in framing zoom-to-fit never goes below. */
	float BaseOrthoWidth = 0.f;
	float CurrentOrthoWidth = 0.f;
};
