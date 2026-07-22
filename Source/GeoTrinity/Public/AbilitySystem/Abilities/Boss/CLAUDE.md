# Abilities/Boss

Boss-specific gameplay abilities.

Hex-arena bosses (`AGeoHexArena`) reach their arena via `AGeoHexArena::GetArenaOfBoss(StoredPayload.Owner)` (arena spawns boss with `Owner = this`, so it's one `GetOwner()` hop). Every tile-touching ability goes through that instead of looking up the arena by class.

Three of the hex boss's six abilities need no C++ class at all — a plain `UPatternAbility` with the right pattern BP suffices (tile-carving ray, cone spray; see `Pattern/CLAUDE.md`). Only the three below need code, to resolve an aim direction, target player, or spawn tile.

---

## `GeoPeriodicFireAbility.h` — server-only periodic projectile fire
Extends `UGeoProjectileAbility`. Auto-activates as passive on ASC assignment. Fires at all players on a random interval (`FireIntervalMin`/`Max`, re-scheduled via `BuildDataAndFire`). ServerOnly, no client prediction. Targets `EProjectileTarget::AllPlayers`.

## `GeoDevastatingWaveAbility.h` — expanding radial wave
Extends `UPatternAbility`. Overrides `GetFireOrigin2D` to teleport the boss to a `TeleportLocationTag`-tagged `AGeoTargetPoint` in its own arena (resolved via `AGeoArena::GetArenaOfBoss` + `GeoLib::GetTargetPoints`); actual teleport happens in `UDevastatingWavePattern::StartPattern`. `TeleportLocationTag` is a `TargetPoint.*` purpose tag only, never an arena tag. Pattern: `UDevastatingWavePattern`.

## `GeoSpawnPillarAbility.h` — fatal-zone pillar launcher
Extends `UPatternAbility`, launches `USpawnPillarPattern`. Overrides `CreatePatternData()` to resolve zone locations once on the server (boss health ratio → 1-3 pillars, players sorted by `PlayerId` for determinism, seeded by `StoredPayload.Seed`) and ships them as `FSpawnPillarPatternData::ZoneLocations` — every client spawns identically instead of recomputing.

## `GeoSweepBeamAbility.h` — beam aimed at arena centre (hex boss)
Extends `UPatternAbility`, launches a `UBeamPattern` BP. Overrides `GetFireYaw` to aim at the arena centre (`AGeoHexArena::GetActorLocation`) instead of the boss's facing, so the sweep always crosses the platform middle. Falls back to boss facing (with `ensureMsgf`) if not a hex boss.

The tile-carving ray needs no ability class: default `GetFireYaw` (boss faces the tank via `STTask_ChaseTarget`) is enough — use a plain `UPatternAbility` + `UBeamPattern` BP with `SweepAngle = 0` + `bDestroyLastTileHit`. Direction locks at activation so the tank can pre-position to choose which rim tile dies.

## `GeoTileBombAbility.h` — bomb stuck to a player (hex boss)
Server-only (`ServerOnly`/`InstancedPerActor`/`ReplicateNo`), like `GeoSpawnOnTileAbility` — **not a pattern**: the bomb is a replicated `AGeoBombZone` deployable, so clients see it without deterministic simulation. `Fire()` draws one live player (`FRandomStream(StoredPayload.Seed)` via `RandHelper`, never `Seed % Num` which goes negative), spawns the bomb at them via `FullySpawnDeployable`, then `AttachToActor` so it rides the carrier; the replicated attachment tracks the authoritative carrier position on every machine, so no client-position RPC is needed. Logs and ends if nobody's alive. The two wind-up phases and the detonation live on the deployable — see `Actor/Deployable/BombZone/CLAUDE.md`.

## `GeoSpawnOnTileAbility.h` — deployables dropped on tiles (hex boss)
Server-only (`ServerOnly`/`InstancedPerActor`/`ReplicateNo`). One class covers both turrets and mines — differ only by deployable class and tile choice. No pattern: a deployable is a replicated actor, clients see it without deterministic simulation. `Fire()` spawns and calls `EndAbility()`.

- `DeployableClass`/`DeployableParams`/`SpawnCount`. `DeployableParams.LifeDrainMaxDuration` arms a mine's fuse (drain→blink→burst); 0 for turrets (live until killed)
- `bSpawnOnHealthScaledRing` — spawn ring = `HealthRatio * GridRadius` (turrets creep in as boss loses health); off for mines (any standing tile). Stateless by design — survives a boss reset
- `GetRandomAliveTiles` returns distinct tiles — multi-spawn never stacks two deployables on one tile
- Calls `SetDeployableInfinitCount` before spawning (deployable cap is meant for player deployables; the arena's tiles are the real limit here)
- Anything spawned here is recalled by the arena when its tile is destroyed — the shared counterplay to both turrets and mines
