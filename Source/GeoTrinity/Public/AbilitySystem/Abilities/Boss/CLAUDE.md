# Abilities/Boss

Boss-specific gameplay abilities.

---

## `GeoPeriodicFireAbility.h` — server-only periodic projectile fire

Extends `UGeoProjectileAbility`. Auto-activates as a passive on ASC assignment. Fires projectiles at all players in a random interval, then re-schedules itself via `BuildDataAndFire`. ServerOnly — no client prediction.

- `FireIntervalMin` / `FireIntervalMax` — interval range in seconds; each shot picks `FMath::FRandRange(Min, Max)` before the next shot
- Targets: `EProjectileTarget::AllPlayers` — resolved in `SpawnProjectilesUsingTarget`

---

## `GeoDevastatingWaveAbility.h` — expanding radial wave

Extends `UPatternAbility`. Teleports the boss to a tagged `AGeoTargetPoint` by overriding `GetFireOrigin2D` (so the payload `Origin` is the destination), then launches `UDevastatingWavePattern`.

- Overrides `GetFireOrigin2D` — resolves the first `AGeoTargetPoint` tagged with `TeleportLocationTag` via `GeoLib::GetTargetPoints` and returns its 2D location (`ensureMsgf` if none match); teleport happens in `UDevastatingWavePattern::StartPattern`
- Effect data configured on the ability via `EffectDataAssets`/`EffectDataInstances`; forwarded automatically to the pattern by `PatternAbility`
- Pattern: `UDevastatingWavePattern` (see `Pattern/CLAUDE.md`)

---

## `GeoSpawnPillarAbility.h` — fatal-zone pillar launcher

Extends `UPatternAbility`. Launches `USpawnPillarPattern`. Overrides `CreatePatternData()` to resolve the zone locations **once on the server** and ship them in an `FSpawnPillarPatternData` through `PatternStartMulticast` — so every client spawns its zones at identical positions instead of recomputing from locally-replicated player state.

- `CreatePatternData()` — reads the boss's health ratio (1–3 pillars), sorts players by `PlayerId` for determinism, picks target locations seeded by `StoredPayload.Seed`, and packs them into `FSpawnPillarPatternData::ZoneLocations`
- Pattern: `USpawnPillarPattern` (see `Pattern/CLAUDE.md`) — reads `ZoneLocations` in `InitPattern`; `PillarClass`/`PillarParams`/`SpawningZoneSize`/`PillarSpawnEffects` are configured on the pattern
