# Actor/Deployable/BuffPickup

`GeoBuffPickup.h` — Triangle reload buff pickup, launched toward `TargetLocation` on `LaunchCurve`.

- One mesh; buff type = colour (dynamic material param) from `BuffIndex` via the reload ability's `BuffColors`; HUD ammo readout uses static `UGeoReloadAbility::GetColorForAmmo`.
- Spawned (reload shower, arena loot) through `GeoASLib::FullySpawnDeployable` with a filled `FBuffPickupData`, like any deployable.
- `PowerScale` scales magnitude (missing ammo at reload).
- `OnOverlap()` server-only after `TimeBeforePickup`; applies `EffectDataArray`, `Recall(false)`.
- Overrides `bUnlimitedDeploy` to false so the reload shower evicts the oldest via `bDestroyOldestWhenLimitReached`.
