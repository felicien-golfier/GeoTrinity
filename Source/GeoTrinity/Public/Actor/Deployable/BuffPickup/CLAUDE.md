# Actor/Deployable/BuffPickup

`GeoBuffPickup.h` — Triangle reload buff pickup, launched toward `TargetLocation` on `LaunchCurve`.

- One mesh; buff type = colour (dynamic material param) from `BuffIndex` via the reload ability's `BuffColors`; HUD ammo readout uses static `UGeoReloadAbility::GetColorForAmmo`.
- `PowerScale` scales magnitude (missing ammo at reload).
- `OnOverlap()` server-only after `TimeBeforePickup`; applies `EffectDataArray`, `Recall(false)`.
- Overrides `bUnlimitedDeploy` to false so the reload shower evicts the oldest via `bDestroyOldestWhenLimitReached`.
