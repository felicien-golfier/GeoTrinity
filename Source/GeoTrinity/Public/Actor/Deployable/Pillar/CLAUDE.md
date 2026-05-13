# Actor/Deployable/Pillar

Boss-fight pillar deployable. Spawned by `GeoFatalZone` when its countdown expires.

## Files
| File | Role |
|---|---|
| `GeoPillar.h/.cpp` | Deployable pillar — has health, no drain, blocks DevastatingWave via collision |

## Key Points
- Extends `AGeoDeployableBase` with `bUseRegularDrain = false` — stays alive until damaged to zero or recalled
- `FPillarData` extends `FDeployableData` (no extra fields for now)
- When health hits zero, `OnHealthChanged_Implementation` calls `Recall()` — never a separate destruction path
- `Recall(Value)` is overridden to act as the "explode" hook; BP overrides `Recall` for death VFX/SFX, then calls Super
- Collision profile inherited from base (`GeoCapsule`); BP sets mesh and capsule size via `Data.Params.Size`
