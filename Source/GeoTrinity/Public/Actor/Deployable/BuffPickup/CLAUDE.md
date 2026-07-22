# Actor/Deployable/BuffPickup

Triangle's buff pickup deployable — spawned on reload.

## Files
| File | Role |
|---|---|
| `GeoBuffPickup.h/.cpp` | Moving buff pickup — launched toward a target, collected on overlap |

## Key Points
- Launched toward `TargetLocation` via `LaunchCurve` on spawn.
- One mesh; buff type conveyed by **color** (dynamic material param), not mesh swap. `BuffIndex` (set by reload ability from remaining ammo) selects the color via the reload ability's CDO/palette (`BuffColors`, BP-editable on the ability, not the pickup). Ammo HUD reads the same palette via static `UGeoReloadAbility::GetColorForAmmo(Ammo)` so the readout matches the next pickup's color without a pickup reference.
- `PowerScale` scales effect magnitude, driven by missing ammo count at reload time.
- `OnOverlap()` — server-only; gated by a `TimeBeforePickup`-second delay from spawn. Applies `EffectDataArray` and calls `Recall(false)`.
