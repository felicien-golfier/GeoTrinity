// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoArena.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoHexArena.generated.h"

class AGeoDeployableBase;
class APlayableCharacter;
class UInstancedStaticMeshComponent;

/** Replicated per-tile state: whether the tile is standing, and whether a telegraph currently tints it. */
USTRUCT()
struct FHexTileState
{
	GENERATED_BODY()

	/** 1 = alive, 0 = destroyed. */
	UPROPERTY()
	uint8 bAlive = 1;

	/** 1 = tinted by at least one telegraph. Union computed server-side (which requester owns which tiles never
	 *  replicates — only this bool does), so overlapping requesters coexist. */
	UPROPERTY()
	uint8 bHighlighted = 0;
};

/**
 * Destructible hexagonal boss platform. Owns a single ISM holding one instance per tile; tiles are pure visuals
 * (no collision) laid over a flat invisible floor — "falling" is game logic, not physics. The replicated TileStates
 * array is the single source of truth; every machine applies it to the ISM in ApplyTileVisuals.
 * Fall checks only run between CommitFight and EndFight: before the fight commits nobody has been teleported onto
 * the platform yet, so anyone standing in the void around it is simply not in the fight.
 */
UCLASS()
class GEOTRINITY_API AGeoHexArena : public AGeoArena
{
	GENERATED_BODY()

public:
	/** Creates the TileMeshComponent ISM and disables tick at start — fall checks are gated on the fight lifecycle. */
	AGeoHexArena();

	/** Registers the replicated tile state array. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/** Rebuilds the grid and the ISM instances whenever the actor is edited or moved in the editor. */
	virtual void OnConstruction(FTransform const& Transform) override;
	/** Server. Kills every player inside FallCheckRadius standing over a destroyed tile or the surrounding void. */
	virtual void Tick(float DeltaSeconds) override;

	/** Starts the fall checks: players are on the platform now. */
	virtual void CommitFight() override;
	/** Stops the fall checks and restores the platform. */
	virtual void EndFight() override;

	/** Server. Destroys the given tiles; holes appear on every machine via replication. Unknown coords are ignored. */
	void DestroyTiles(TConstArrayView<FIntPoint> Tiles);
	/** Returns the ISM instance indices of all tiles whose centers lie within Radius world units of Center. */
	TArray<int> GetTilesIndexInRadius(FVector2D Center, float Radius);
	/** Server. Destroys every tile whose center lies within Radius world units of Center. */
	void DestroyTilesInRadius(FVector2D Center, float Radius);
	/** Server. Restores every tile. Called when the boss is defeated or the group wipes. */
	void ResetAllTiles();

	/** Returns true when Tile exists on the platform and has not been destroyed. */
	bool IsTileAlive(FIntPoint Tile) const;
	/** Returns true when a still-living deployable occupies Tile (recorded by SetTileOccupant). */
	bool IsTileOccupied(FIntPoint Tile) const;
	/** Returns true when WorldLocation stands over a tile that is still up — false over a hole or off the platform. */
	bool IsOverAliveTile(FVector2D WorldLocation) const;
	/** Maps a world location to the tile containing it. Returns false when the location is outside the platform. */
	bool GetTileUnderLocation(FVector2D WorldLocation, FIntPoint& OutTile) const;
	/** Returns true when WorldLocation stands over a tile that is still up — false over a hole or off the platform. */
	bool IsOverAliveTile(FVector2D WorldLocation, FIntPoint& OutTile) const;
	/** Returns the world-space center of Tile. */
	FVector2D TileToWorld(FIntPoint Tile) const;

	/** Returns the arena that spawned Boss — every arena owns the boss it spawns. Null when Boss is not one of them. */
	static AGeoHexArena* GetArenaOfBoss(AActor const* Boss);
	/** Hex-disc radius in rings: 0 is the single center tile, GetGridRadius() the outer ring. */
	int32 GetGridRadius() const { return GridRadius; }
	/** Returns the ring Tile sits on, counted outward from the center tile. */
	static int32 GetTileRing(FIntPoint Tile);

	/**
	 * Picks up to Count distinct tiles that are still standing and not already occupied by a deployable.
	 *
	 * @param Ring  Ring to draw from; the whole platform when negative, and also when that ring has nothing left.
	 */
	TArray<FIntPoint> GetRandomAliveTiles(FRandomStream& Stream, int32 Ring, int32 Count) const;

	/** Server. Records Deployable as the occupant of Tile so GetRandomAliveTiles skips it. Cleared automatically when
	 * the deployable is destroyed (weak ptr goes stale) and on arena reset. */
	void SetTileOccupant(FIntPoint Tile, AGeoDeployableBase* Deployable);
	/** Returns the furthest still-standing tile the ray crosses within MaxRange. False when it crosses none. */
	bool GetLastAliveTileAlongRay(FVector2D Origin, FVector2D Direction, FIntPoint& OutTile) const;

	/** Server. Highlights the tiles within Radius of Location for Requester, or the single tile under Location when
	 * Radius is 0. */
	void HighlightTiles(AActor* Requester, FVector2D Location, float Radius = 0.f);
	/** Server. Highlights the single tile at Tile for Requester, replacing any previous highlight from that requester. */
	void HighlightTile(AActor* Requester, FIntPoint Tile);
	/** Server. Drops Requester's highlight. */
	void ClearHighlight(AActor* Requester);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HexArena")
	TObjectPtr<UInstancedStaticMeshComponent> TileMeshComponent;

	/** Hex-disc radius in rings; tile count = 1 + 3 * R * (R + 1) (5 rings = 91 tiles). */
	UPROPERTY(EditAnywhere, Category = "HexArena", meta = (ClampMin = "1"))
	int32 GridRadius = 5;

	/** Outer radius of one hexagon tile in world units (center to corner). */
	UPROPERTY(EditAnywhere, Category = "HexArena", meta = (ClampMin = "1.0"))
	float TileSize = 100.f;

	/** Players within this distance of the arena center are subject to the fall check. Must cover the platform plus
	 * the surrounding void, and exclude the rest of the map. */
	UPROPERTY(EditAnywhere, Category = "HexArena", meta = (ClampMin = "0.0"))
	float FallCheckRadius = 3000.f;

	/** Grace distance in world units: an actor whose center is over a hole still stands as long as an alive tile lies
	 * within this radius, so perching on a neighbouring tile's edge doesn't drop you. 0 = center must be over a tile. */
	UPROPERTY(EditAnywhere, Category = "HexArena", meta = (ClampMin = "0.0"))
	float FallGraceMargin = 30.f;

private:
	/** The fall check: true when WorldLocation is over an alive tile, or within FallGraceMargin of the shared border of
	 * the alive neighbour it leans toward (perpendicular distance, so the grace band is uniform around the tile). */
	bool IsSupported(FVector2D WorldLocation) const;
	/** Converts a world location to fractional axial (Q, R) hex coordinates in this arena's frame. */
	FVector2D WorldToAxial(FVector2D WorldLocation) const;
	/** Kills Player and moves the corpse off the platform, to this arena's TargetPoint.FallRespawn point. */
	void KillFallenPlayer(APlayableCharacter& Player) const;
	/** Deterministically fills TileCoords / CoordToIndex for the configured GridRadius. Idempotent. */
	void BuildGrid();
	/** Applies TileStates (alive scale + highlight custom data) to the ISM instances, diffing against
	 * AppliedTileStates. */
	void ApplyTileVisuals();
	/** Records Requester's highlighted tile indices (empty = drop it), then refreshes. Server-only core of the
	 * highlight API. */
	void SetHighlightedTiles(AActor* Requester, TConstArrayView<int32> Indices);
	/** Recomputes every tile's bHighlighted from the union of live HighlightRequests, drops stale ones, then applies.
	 */
	void RefreshHighlightStates();
	/** Returns the actor-space center of Tile (pointy-top axial layout). */
	FVector TileToLocal(FIntPoint Tile) const;
	FTransform GetTileTransform(FVector const& TileLocation) const;

	UFUNCTION()
	void OnRep_TileStates();

	/** Drops the expired deployable from TileOccupants so its tile is immediately available again — bound to each
	 * occupant's OnDeployableExpiredEvent, which fires at Expire() ahead of the delayed Destroy(). */
	UFUNCTION()
	void OnTileOccupantExpired(AGeoDeployableBase* Deployable);

	/** Alive + highlight state per tile; indexed like TileCoords. Single source of truth, replicated. */
	UPROPERTY(ReplicatedUsing = OnRep_TileStates)
	TArray<FHexTileState> TileStates;

	/** Last states applied to the ISM on this machine; lets ApplyTileVisuals touch only changed instances. */
	TArray<FHexTileState> AppliedTileStates;
	TArray<FIntPoint> TileCoords;
	TMap<FIntPoint, int32> CoordToIndex;

	/** Server-only: each requester's highlighted tile indices. Their union drives the replicated bHighlighted bit. */
	TMap<TWeakObjectPtr<AActor>, TArray<int32>> HighlightRequests;

	/** Server-only: deployable currently sitting on each tile. Stale entries (destroyed deployables) read as vacant. */
	TMap<FIntPoint, TWeakObjectPtr<AGeoDeployableBase>> TileOccupants;
};
