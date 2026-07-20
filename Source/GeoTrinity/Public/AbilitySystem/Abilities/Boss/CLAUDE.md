# Abilities/Boss

Boss-specific gameplay abilities.

**Hex-arena bosses** (`AGeoHexArena`) reach their arena with `AGeoHexArena::GetArenaOfBoss(StoredPayload.Owner)` —
an arena spawns its boss with `Owner = this`, so the arena is one `GetOwner()` hop from the boss. Every ability that
touches tiles goes through that; none of them look the arena up by class.

Three of the hex boss's six abilities need **no C++ ability class at all** — a plain `UPatternAbility` with the right
pattern BP is enough (the tile-carving ray and the cone spray; see `Pattern/CLAUDE.md`). Only the three below need
code, because they resolve something the pattern cannot: an aim direction, a target player, or a spawn tile.

---

## `GeoPeriodicFireAbility.h` — server-only periodic projectile fire

Extends `UGeoProjectileAbility`. Auto-activates as a passive on ASC assignment. Fires projectiles at all players in a random interval, then re-schedules itself via `BuildDataAndFire`. ServerOnly — no client prediction.

- `FireIntervalMin` / `FireIntervalMax` — interval range in seconds; each shot picks `FMath::FRandRange(Min, Max)` before the next shot
- Targets: `EProjectileTarget::AllPlayers` — resolved in `SpawnProjectilesUsingTarget`

---

## `GeoDevastatingWaveAbility.h` — expanding radial wave

Extends `UPatternAbility`. Teleports the boss to a tagged `AGeoTargetPoint` by overriding `GetFireOrigin2D` (so the payload `Origin` is the destination), then launches `UDevastatingWavePattern`.

- Overrides `GetFireOrigin2D` — resolves the boss's arena via `AGeoArena::GetArenaOfBoss`, then the first point carrying `TeleportLocationTag` **in that arena** (`GeoLib::GetTargetPoints(Instigator, TeleportLocationTag, Arena->ArenaTag)`) and returns its 2D location (`ensureMsgf` on either miss); teleport happens in `UDevastatingWavePattern::StartPattern`. `TeleportLocationTag` is a `TargetPoint.*` purpose only (`TargetPoint.BossSpawn` on `GA_DevastatingWave`) — never an arena tag, since the arena comes from the boss
- Effect data configured on the ability via `EffectDataAssets`/`EffectDataInstances`; forwarded automatically to the pattern by `PatternAbility`
- Pattern: `UDevastatingWavePattern` (see `Pattern/CLAUDE.md`)

---

## `GeoSpawnPillarAbility.h` — fatal-zone pillar launcher

Extends `UPatternAbility`. Launches `USpawnPillarPattern`. Overrides `CreatePatternData()` to resolve the zone locations **once on the server** and ship them in an `FSpawnPillarPatternData` through `PatternStartMulticast` — so every client spawns its zones at identical positions instead of recomputing from locally-replicated player state.

- `CreatePatternData()` — reads the boss's health ratio (1–3 pillars), sorts players by `PlayerId` for determinism, picks target locations seeded by `StoredPayload.Seed`, and packs them into `FSpawnPillarPatternData::ZoneLocations`
- Pattern: `USpawnPillarPattern` (see `Pattern/CLAUDE.md`) — reads `ZoneLocations` in `InitPattern`; `PillarClass`/`PillarParams`/`SpawningZoneSize`/`PillarSpawnEffects` are configured on the pattern

---

## `GeoSweepBeamAbility.h` — beam aimed at the arena centre (hex boss)

Extends `UPatternAbility`; launches a `UBeamPattern` BP. Its only job is `GetFireYaw` → yaw from the boss toward the
arena centre (`AGeoHexArena::GetActorLocation`, which *is* the grid origin), instead of the boss's own facing. The
sweep therefore always crosses the middle of the platform and the ground behind the boss stays the safe side, which
is what forces players around it. Falls back to the boss's facing (with an `ensureMsgf`) when it is not a hex boss.

The **tile-carving ray** needs no ability class: the boss already faces the tank (`STTask_ChaseTarget`), so the
default `GetFireYaw` already aims at them. Use a plain `UPatternAbility` with a `UBeamPattern` BP configured
`SweepAngle = 0` + `bDestroyLastTileHit`. Direction is locked at activation, so the tank pre-positions to choose
which rim tile dies — the telegraph shows an honest line.

## `GeoTileBombAbility.h` — bomb stuck to a player (hex boss)

Extends `UPatternAbility`; launches `UTileBombPattern`. `CreatePatternData()` draws one live player from
`GetInteractableActors<APlayableCharacter>` using `FRandomStream(StoredPayload.Seed)` and ships it as
`FTileBombPatternData::BombCarrier`. Resolved **once on the server** for the same reason as `GeoSpawnPillarAbility`:
every machine must mark the same player. `RandHelper` — never `Seed % Num`, which goes negative on a negative seed.
Logs and ships an empty carrier when nobody is alive (the pattern then fizzles).

## `GeoSpawnOnTileAbility.h` — deployables dropped on tiles (hex boss)

Server-only (`ServerOnly` / `InstancedPerActor` / `ReplicateNo`, like `GeoPeriodicFireAbility`). **One class covers
both the turrets and the mines** — they are the same operation, differing only by deployable class and tile choice.
No pattern involved: a deployable is a replicated actor, so clients see it without any deterministic client-side
simulation. `Fire()` spawns and calls `EndAbility()` so the StateTree task completes.

- `DeployableClass` / `DeployableParams` / `SpawnCount` — what to drop and how much. `DeployableParams.LifeDrainMaxDuration` is what **arms a mine's fuse** (drain to zero → blink → burst); leave it 0 for turrets, which live until killed
- `bSpawnOnHealthScaledRing` — spawn ring = `HealthRatio * GridRadius`, so turrets creep in from the rim as the boss loses health. Off (the mines) draws from any still-standing tile. Stateless on purpose: it survives a boss reset, unlike a per-activation counter would
- `GetRandomAliveTiles` returns **distinct** tiles, so a multi-spawn never stacks two deployables on one tile
- Calls `SetDeployableInfinitCount` before spawning, like `USpawnPillarPattern` does — the boss's deployable cap is meant for player deployables; here the arena's tiles are the real limit
- Anything spawned this way is recalled by the arena when its tile is destroyed, which is the shared counterplay to turrets and mines alike
