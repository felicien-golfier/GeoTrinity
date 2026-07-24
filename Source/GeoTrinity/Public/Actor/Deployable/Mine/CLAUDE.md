# Actor/Deployable/Mine

Hex-boss mine deployable. Dropped on an arena tile by `UGeoSpawnOnTileAbility`.

## Files
| File | Role |
|---|---|
| `GeoMine.h/.cpp` | Timed mine — bursts projectiles in every direction, unless its tile is gone |

## Key Points
- **Health is the mine's life; a separate fuse detonates it.** `bUseRegularDrain = false` (health only drops from player damage) + `bAutoRecallAtEndLife = true`. `InitDrain` (repurposed) arms a `FuseTimerHandle` of `LifeDrainMaxDuration` instead of the base life drain. Two non-overlapping end paths:
  - **Players destroy it (health→0):** `OnHealthChanged` override calls `Expire()` — defuse, never bursts.
  - **Fuse runs out first:** `OnFuseElapsed` → `StartBlinking()` → base blink-end → `Recall()` → `RecallEffect()` bursts. Total fuse to detonation = `LifeDrainMaxDuration + BlinkDuration`.
  - No race: `StartBlinking()` disables damage, so health can't reach 0 during the explode-blink; the fuse guards on `bActive`, so a mine that already expired is left alone.
- `RecallEffect(Value)` (server-only) spawns `BurstProjectileCount` projectiles evenly around the circle, same route `AGeoTurret::TryFire` uses. Reached only via the fuse path.
- **A mine dropped into the void never detonates** — `AGeoHexArena`'s fall-check `Expire()`s (not `Recall()`s) any deployable over a dead tile, so losing the tile defuses without ever running `RecallEffect`.
- `MineData` replicated `COND_InitialOnly`, like `AGeoPillar`/`AGeoTurret`.
