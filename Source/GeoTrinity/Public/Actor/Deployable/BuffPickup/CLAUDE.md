# Actor/Deployable/BuffPickup

Triangle's buff pickup deployable — spawned on reload.

## Files
| File | Role |
|---|---|
| `GeoBuffPickup.h/.cpp` | Moving buff pickup — launched toward a target, collected on overlap |

## Key Points
- Launched toward `TargetLocation` via `LaunchCurve` on spawn
- Single mesh set on `BuffMeshComponent` in the BP; buff type is conveyed by **color**, not by swapping meshes
- `BuffIndex` (set by the reload ability from remaining ammo) selects the color. `UpdateColor()` resolves the reload ability CDO from `Data.AbilityTag` via `GeoASLib::GetAbilityCDO<UGeoReloadAbility>` and reads `GetColorForIndex(BuffIndex)`, written to the dynamic material's `ColorParameterName` vector param (runs in `BeginPlay` and `OnRep_Data`). The palette (`BuffColors`) lives on the **ability** (BP-editable), not on the pickup or a global — the ammo HUD readout reads the same palette via the **static** `UGeoReloadAbility::GetColorForAmmo(Ammo)` (`BlueprintPure`), which resolves the reload CDO itself from the `Ability.Type.Reload` tag, so the overlay BP needs only the ammo value (no ability reference) and the number matches the next reload's pickup color.
- `PowerScale` scales effect magnitude — driven by missing ammo count at reload time
- `OnOverlap()` — server-only; guarded by a `TimeBeforePickup`-second delay from spawn (`StartTime` is recorded in `InitInteractable` so the timer is consistent on the listen-server host before `BeginPlay`). Once the guard passes, applies `EffectDataArray` to the collecting actor and calls `Recall(false)`
