// Copyright 2024 GeoTrinity. All Rights Reserved.

#pragma once

#include "Actor/GeoArena.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "GeoHexArena.generated.h"

class APlayableCharacter;
class UInstancedStaticMeshComponent;

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
	/** Server. Destroys every tile whose center lies within Radius world units of Center. */
	void DestroyTilesInRadius(FVector2D Center, float Radius);
	/** Server. Restores every tile. Called when the boss is defeated or the group wipes. */
	void ResetAllTiles();

	/** Returns true when Tile exists on the platform and has not been destroyed. */
	bool IsTileAlive(FIntPoint Tile) const;
	/** Maps a world location to the tile containing it. Returns false when the location is outside the platform. */
	bool GetTileUnderLocation(FVector2D WorldLocation, FIntPoint& OutTile) const;
	/** Returns true when WorldLocation stands over a tile that is still up — false over a hole or off the platform. */
	bool IsOverAliveTile(FVector2D WorldLocation) const;
	/** Returns the world-space center of Tile. */
	FVector2D TileToWorld(FIntPoint Tile) const;

	/** Returns the arena that spawned Boss — every arena owns the boss it spawns. Null when Boss is not one of them. */
	static AGeoHexArena* GetArenaOfBoss(AActor const* Boss);
	/** Hex-disc radius in rings: 0 is the single center tile, GetGridRadius() the outer ring. */
	int32 GetGridRadius() const { return GridRadius; }
	/** Returns the ring Tile sits on, counted outward from the center tile. */
	static int32 GetTileRing(FIntPoint Tile);

	/**
	 * Picks up to Count distinct tiles that are still standing.
	 *
	 * @param Ring  Ring to draw from; the whole platform when negative, and also when that ring has nothing left.
	 */
	TArray<FIntPoint> GetRandomAliveTiles(FRandomStream& Stream, int32 Ring, int32 Count) const;
	/** Returns the furthest still-standing tile the ray crosses within MaxRange. False when it crosses none. */
	bool GetLastAliveTileAlongRay(FVector2D Origin, FVector2D Direction, float MaxRange, FIntPoint& OutTile) const;

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

private:
	/** Kills Player and moves the corpse off the platform, to this arena's TargetPoint.FallRespawn point. */
	void KillFallenPlayer(APlayableCharacter& Player) const;
	/** Deterministically fills TileCoords / CoordToIndex for the configured GridRadius. Idempotent. */
	void BuildGrid();
	/** Applies TileStates to the ISM instances, diffing against AppliedTileStates. Runs on every machine. */
	void ApplyTileVisuals();
	/** Returns the actor-space center of Tile (pointy-top axial layout). */
	FVector TileToLocal(FIntPoint Tile) const;
	FTransform GetTileTransform(FVector const& TileLocation) const;

	UFUNCTION()
	void OnRep_TileStates();

	/** 1 = alive, 0 = destroyed; indexed like TileCoords. */
	UPROPERTY(ReplicatedUsing = OnRep_TileStates)
	TArray<uint8> TileStates;

	/** Last states applied to the ISM on this machine; lets ApplyTileVisuals touch only changed instances. */
	TArray<uint8> AppliedTileStates;
	TArray<FIntPoint> TileCoords;
	TMap<FIntPoint, int32> CoordToIndex;
};
