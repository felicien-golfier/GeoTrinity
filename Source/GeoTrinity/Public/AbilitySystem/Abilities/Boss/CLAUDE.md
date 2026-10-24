# Abilities/Boss

Server-only boss abilities. Reach the arena via `AGeoArena::GetArenaOfBoss(StoredPayload.Owner)`; never look it up by class.
A plain `UPatternAbility` + pattern Blueprint needs no class here (tile-carving ray, cone spray).

| File | Role |
|---|---|
| `GeoPeriodicPatternAbility.h` | Never ends: each `USalvePattern` launch is one salve at the caster's hostiles (enemy team: all players; player team: the boss of its arena), aimed and stamped right before it leaves, then schedules the next |
| `GeoSpawnPillarAbility.h` | Resolves pillar zone locations once (`CreatePatternData`): health ratio → 1-3 zones, players sorted by `PlayerId`, seeded |
| `GeoSweepBeamAbility.h` | Beam aimed at the hex arena centre (`GetFireYaw` override) |
| `GeoTileBombAbility.h` | Draws a live player, spawns `AGeoBombZone` attached to them; not a pattern (replicated deployable) |
| `GeoSpawnOnTileAbility.h` | Turret / mine drop on arena tiles; ServerOnly, not a pattern |
| `GeoSummonAddAbility.h` | `UPatternAbility`: its zone pattern telegraphs a spot near a live player (Damage colour, lands its hit), then `Arena->SpawnAdd(AddClass, AddSetup)` there |

## Montages
- Bomb and turret have no pattern, so their `AnimMontage` replicates (`Base/CLAUDE.md`, "Montage ownership"). Authored in the `Cast` slot group, apart from the pattern montages' group, so a cast never cuts a running pattern montage.

## GeoSpawnOnTileAbility
- `DeployableClass`, `DeployableParams`, `SpawnCount`; `LifeDrainMaxDuration` arms a mine fuse, 0 for turrets.
- `bSpawnOnHealthScaledRing`: ring radius = `HealthRatio * GridRadius`; stateless so it survives boss reset.
- Tiles via `GetRandomAliveTiles` (distinct, unoccupied); `Fire` marks `SetTileOccupant`.
- Deployables unlimited by default; arena recalls them when their tile dies.

## Tile bomb
- Random pick: `FRandomStream(Seed)` + `RandHelper`.
- Two phases live on `AGeoBombZone` (`Actor/Deployable/BombZone`).

## GeoSummonAddAbility
- Spot = `GetFireOrigin2D`: a live player (seeded pick) plus an offset up to `SpotRadius`; the caster when none lives. Stamped in the payload, so the telegraph and the add agree.
- `PatternToLaunch` = a `UZonePattern` Blueprint (telegraph in the Damage colour, then its effects): the hit lands on whoever stays.
- The add spawns in `OnPatternEnd` (server), only on a natural end: a force-stopped pattern (wipe, boss death) brings nothing.
- `AddSetup` (`FGeoEnemySetup`): the add's variant (health row, tree, abilities), so the add class holds only its look and its own behaviour's params.
- `PatternToLaunch` is any zone pattern as is (tutorial: the Training Room's `BP_ZonePattern_BurstDamage`); the hit's numbers are the ability's `EffectDataInstances`.

## GeoPeriodicPatternAbility
- Aim from the caster's team, no flag: enemy team → one yaw per live player; player team (an ally add shooting the boss, tutorial) → one yaw at `AGeoArena::Boss` of its arena (`GetArenaOfBoss(caster)`), none (nothing fired) when it has no arena or the boss is dead.
- One salve ability serves a boss, its summons and their allies (tutorial: `GA_TutorialSalve`); its pattern BP's bullets are `OverlapAttitude` hostile only, so each side hits only the other.
