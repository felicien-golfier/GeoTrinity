# Actor/Deployable/BuffPickup

Triangle's buff pickup deployable — spawned on reload.

## Files
| File | Role |
|---|---|
| `GeoBuffPickup.h/.cpp` | Moving buff pickup — launched toward a target, collected on overlap |

## Key Points
- Launched toward `TargetLocation` via `LaunchCurve` on spawn
- Single mesh set on `BuffMeshComponent` in the BP; buff type is conveyed by **color**, not by swapping meshes
- `BuffIndex` (set by the reload ability from remaining ammo) selects the color. `UpdateColor()` resolves the reload ability CDO from `Data.AbilityTag` via `GeoASLib::GetAbilityCDO<UGeoReloadAbility>` and reads `GetColorForIndex(BuffIndex)`, written to the dynamic material's `ColorParameterName` vector param (runs in `BeginPlay` and `OnRep_Data`). The palette (`BuffColors`) lives on the **ability** (BP-editable), not on the pickup or a global — the ammo HUD readout reads the same CDO via `GetColorForAmmo` so the number matches the next reload's pickup color.
- `PowerScale` scales effect magnitude — driven by missing ammo count at reload time
- `OnOverlap()` — applies `EffectDataArray` to the collecting actor, then calls `Recall(false)`
