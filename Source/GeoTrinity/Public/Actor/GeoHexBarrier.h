// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoArenaBarrier.h"
#include "CoreMinimal.h"

#include "GeoHexBarrier.generated.h"

class UInstancedStaticMeshComponent;
class UCurveVector;

/**
 * Arena barrier whose blocking element is an alley floor of hex tiles matching AGeoHexArena's platform.
 * Instead of moving, the alley vanishes tile by tile while the barrier closes and reappears tile by tile while it
 * opens, driven by the inherited open/close lerp. Tiles are pure visuals with no collision, like the arena's.
 */
UCLASS()
class GEOTRINITY_API AGeoHexBarrier : public AGeoArenaBarrier
{
	GENERATED_BODY()

public:
	/** Creates the TileMeshComponent ISM as the root component and enables continuous Tick capability. */
	AGeoHexBarrier();

	/** Rebuilds the alley's ISM instances whenever the actor is edited or moved in the editor. */
	virtual void OnConstruction(FTransform const& Transform) override;
	/** Advances the inherited lerp, then hides/shows tiles so the hidden count tracks the lerp progress. */
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier")
	TObjectPtr<UInstancedStaticMeshComponent> TileMeshComponent;

	/** Vanish sweep: column by column along local +X when true, row by row along local +Y when false. */
	UPROPERTY(EditAnywhere, Category = "Barrier")
	bool bVanishAlongColumns = true;

	/** Tiles along the alley (local +X). */
	UPROPERTY(EditAnywhere, Category = "Barrier", meta = (ClampMin = "1"))
	int32 NumColumns = 8;

	/** Tiles across the alley (local +Y). */
	UPROPERTY(EditAnywhere, Category = "Barrier", meta = (ClampMin = "1"))
	int32 NumRows = 3;

	/** Outer radius of one hexagon tile in world units (center to corner). Match the arena's TileSize. */
	UPROPERTY(EditAnywhere, Category = "Barrier", meta = (ClampMin = "1.0"))
	float TileSize = 100.f;

	/**
	 * Drives the pre-vanish tile shake, sampled by the inherited LerpAlpha (0 = removal just started, 1 = fully
	 * closed): X = offset amplitude, Y = oscillation speed (radians/sec). Lets designers ramp the shake up (or
	 * taper it) as the barrier finishes closing.
	 */
	UPROPERTY(EditAnywhere, Category = "Barrier")
	TObjectPtr<UCurveVector> ShakeCurve;

private:
	/** Actor-space transform of the Index-th tile in vanish order — pointy-top layout, odd rows shifted half a tile. */
	FTransform GetTileTransform(int32 Index) const;
	/**
	 * Zero-scales the first LerpAlpha * NumTiles tiles and restores the rest, touching only changed instances.
	 * While bRemoving, shakes every still-visible tile together so their imminent vanish reads clearly.
	 */
	void ApplyTileVisuals(bool bRemoving);

	/** Number of tiles currently hidden on this machine; index 0 vanishes first. */
	int32 AppliedHiddenCount = 0;
	/** Whether the still-visible tiles were shaking last tick; used to restore them once shaking stops. */
	bool bWasShaking = false;
};
