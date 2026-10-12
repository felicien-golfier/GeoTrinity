# Abilities/Boss

Server-only boss abilities. Reach the arena via `AGeoArena::GetArenaOfBoss(StoredPayload.Owner)`; never look it up by class.
A plain `UPatternAbility` + pattern Blueprint needs no class here (tile-carving ray, cone spray).

| File | Role |
|---|---|
| `GeoPeriodicPatternAbility.h` | Never ends: each `USalvePattern` launch is one salve at all players, aimed and stamped right before it leaves, then schedules the next |
| `GeoSpawnPillarAbility.h` | Resolves pillar zone locations once (`CreatePatternData`): health ratio → 1-3 zones, players sorted by `PlayerId`, seeded |
| `GeoSweepBeamAbility.h` | Beam aimed at the hex arena centre (`GetFireYaw` override) |
| `GeoTileBombAbility.h` | Draws a live player, spawns `AGeoBombZone` attached to them; not a pattern (replicated deployable) |
| `GeoSpawnOnTileAbility.h` | Turret / mine drop on arena tiles; ServerOnly, not a pattern |

## GeoSpawnOnTileAbility
- `DeployableClass`, `DeployableParams`, `SpawnCount`; `LifeDrainMaxDuration` arms a mine fuse, 0 for turrets.
- `bSpawnOnHealthScaledRing`: ring radius = `HealthRatio * GridRadius`; stateless so it survives boss reset.
- Tiles via `GetRandomAliveTiles` (distinct, unoccupied); `Fire` marks `SetTileOccupant`.
- Deployables unlimited by default; arena recalls them when their tile dies.

## Tile bomb
- Random pick: `FRandomStream(Seed)` + `RandHelper`.
- Two phases live on `AGeoBombZone` (`Actor/Deployable/BombZone`).
