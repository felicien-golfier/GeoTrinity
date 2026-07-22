# Actor/Deployable/Mine

Hex-boss mine deployable. Dropped on an arena tile by `UGeoSpawnOnTileAbility`.

## Files
| File | Role |
|---|---|
| `GeoMine.h/.cpp` | Timed mine — bursts projectiles in every direction, unless its tile is gone |

## Key Points
- **The drain is the fuse.** `bUseRegularDrain = true` + `bAutoRecallAtEndLife = true` — base-class drain runs the countdown and hands off to `Recall()`. Total fuse = `LifeDrainMaxDuration + BlinkDuration` (both set by the spawning ability); no separate timer.
- `RecallEffect(Value)` (server-only) spawns `BurstProjectileCount` projectiles evenly around the circle, same route `AGeoTurret::TryFire` uses.
- **A mine with no ground under it never detonates** — `RecallEffect` returns early when `AGeoHexArena::IsOverAliveTile` is false, so the arena's fall-check `Recall()` defuses instead of triggers it. Damaging the mine only brings the burst forward; taking its tile away is the real counterplay.
- `MineData` replicated `COND_InitialOnly`, like `AGeoPillar`/`AGeoTurret`.
