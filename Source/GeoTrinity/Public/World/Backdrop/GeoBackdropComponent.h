// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"

#include "GeoBackdropComponent.generated.h"

class UMaterialInterface;
class UStaticMesh;

/**
 * Builds the parallax star backdrop: one plane per entry in Layers, stacked behind the camera and attached to it.
 *
 * The planes ride the camera rather than sitting in the level because orthographic projection gives them no parallax
 * of their own — a plane one metre behind the floor and one ten kilometres behind it translate on screen by exactly
 * the same amount. All of the depth comes from M_ParallaxStars offsetting its UVs against the camera position
 * AGeoGameCamera publishes into MPC_Camera, so the geometry only has to cover the view and never has to move.
 *
 * Layers is the whole layering: each material is a MI_ParallaxStars instance with its own Parallax and ZoomImmunity,
 * and this component only decides where the planes sit and in what order they draw. Purely cosmetic — skipped
 * entirely on a dedicated server, never ticks.
 */
UCLASS(ClassGroup = (Geo), meta = (BlueprintSpawnableComponent))
class GEOTRINITY_API UGeoBackdropComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Disables tick — the backdrop planes are created once in BeginPlay and need no per-frame update. */
	UGeoBackdropComponent();

	/** Creates and attaches one plane per layer. */
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoBackdrop")
	bool Active = true;
	
	/** Plane the layers are drawn on (/Engine/BasicShapes/Plane). Its own size is read from its bounds, so any flat
	 * mesh facing +Z works. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoBackdrop")
	TObjectPtr<UStaticMesh> LayerMesh;

	/** MI_ParallaxStars instances, farthest first: entry 0 draws behind every other one. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GeoBackdrop")
	TArray<TObjectPtr<UMaterialInterface>> Layers;

	/** World size each plane is scaled to. Must cover the view at MaxOrthoWidth, or the backdrop stops short of the
	 * screen edge once the camera zooms out for couch coop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoBackdrop", meta = (ClampMin = "1"))
	float LayerSize = 8000.f;

	/** Distance from the camera to the first plane, along the view direction — so it must be larger than the camera's
	 * own height above the play plane, or the layers draw in front of the floor and their additive light washes over
	 * the whole scene instead of showing through the lattice. Bounded the other way by the ortho far clip plane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoBackdrop", meta = (ClampMin = "0"))
	float FirstLayerDistance = 6000.f;

	/** Gap between consecutive planes — depth ordering only; orthographic projection makes the distance itself
	 * invisible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GeoBackdrop", meta = (ClampMin = "1"))
	float LayerSpacing = 100.f;
};
