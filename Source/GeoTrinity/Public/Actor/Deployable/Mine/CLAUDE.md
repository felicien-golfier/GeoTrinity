# Actor/Deployable/Mine

`GeoMine.h` — hex-boss mine dropped on a tile by `UGeoSpawnOnTileAbility`.

- Health is the mine's life, a separate fuse detonates: `bUseRegularDrain = false`, `bAutoRecallAtEndLife = true`.
- `InitDrain` arms `FuseTimerHandle` (`LifeDrainMaxDuration`); detonation after fuse + `BlinkDuration`.
- Health → 0: `OnHealthChanged` → `Expire()` = defused, no burst.
- Fuse first: `OnFuseElapsed` → `StartBlinking()` → `Recall()` → `RecallEffect()` bursts `BurstProjectileCount` projectiles evenly (same route as `AGeoTurret::TryFire`).
- Losing its tile `Expire()`s it (defused).
- `MineData` `COND_InitialOnly`.
