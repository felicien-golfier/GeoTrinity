# Actor/Deployable/Mine

Hex-boss mine deployable. Dropped on an arena tile by `UGeoSpawnOnTileAbility`.

## Files
| File | Role |
|---|---|
| `GeoMine.h/.cpp` | Timed mine — bursts projectiles in every direction, unless its tile is gone |

## Key Points
- **The drain is the fuse.** `bUseRegularDrain = true` + `bAutoRecallAtEndLife = true` in the constructor, so the
  base-class drain runs the countdown and hands off to `Recall()` at the end. Total fuse =
  `Params.LifeDrainMaxDuration + Params.BlinkDuration`, both set by the spawning ability; the blink is the
  about-to-blow telegraph, and it's free. No timer of its own.
- `RecallEffect(Value)` override — server-only path (clients get `Expire()` via `OnRep_Active`). Spawns
  `BurstProjectileCount` projectiles evenly around the circle via `StartSpawnProjectile`/`FinishSpawnProjectile`,
  the same server-authoritative route `AGeoTurret::TryFire` uses.
- **A mine with no ground under it never detonates.** `RecallEffect` returns early when
  `AGeoHexArena::IsOverAliveTile` is false. That is what makes destroying the tile a real defuse instead of a
  trigger — the arena's fall check `Recall()`s the mine the moment its tile dies, and without this check that recall
  would set it off. It also keeps the base-class invariant intact (everything ends through `Recall()`, never
  `Expire()` directly).
- Damaging the mine only brings the burst *forward* — health reaching zero is still a `Recall()` on live ground.
  Shooting it is not the counterplay; taking its tile away is.
- `MineData` (`FDeployableData`) replicated `COND_InitialOnly`, like `AGeoPillar`/`AGeoTurret`.
