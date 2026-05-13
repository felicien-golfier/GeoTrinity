# Actor/Deployable/BuffPickup

Triangle's buff pickup deployable — spawned on reload.

## Files
| File | Role |
|---|---|
| `GeoBuffPickup.h/.cpp` | Moving buff pickup — launched toward a target, collected on overlap |

## Key Points
- Launched toward `TargetLocation` via `LaunchCurve` on spawn
- `MeshIndex` selects which `BuffMeshAssets` mesh to display (matches buff type)
- `PowerScale` scales effect magnitude — driven by missing ammo count at reload time
- `OnOverlap()` — applies `EffectDataArray` to the collecting actor, then calls `Recall(false)`
