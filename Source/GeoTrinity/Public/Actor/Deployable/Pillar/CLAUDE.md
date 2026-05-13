# Actor/Deployable/Pillar

Boss-fight pillar deployable. Spawned by `GeoFatalZone` when its countdown expires.

## Files
| File | Role |
|---|---|
| `GeoPillar.h/.cpp` | Deployable pillar — has health, no drain, blocks DevastatingWave via collision |

## Key Points
- Extends `AGeoDeployableBase` with `bUseRegularDrain = false` — stays alive until damaged to zero or recalled
- `FPillarData` extends `FDeployableData` (no extra fields for now)
- `OnPillarDestroyed` — `BlueprintNativeEvent`; override in BP for death VFX/SFX
- Collision profile inherited from base (`GeoCapsule`); BP sets mesh and capsule size via `Data.Params.Size`
- `OnHealthChanged_Implementation` calls `OnPillarDestroyed()` when health hits zero (before base Expire logic)
