# Actor/Deployable/Pillar

Boss-fight pillar deployable. Spawned by `GeoFatalZone` when its countdown expires.

## Files
| File | Role |
|---|---|
| `GeoPillar.h/.cpp` | Deployable pillar — has health, no drain, blocks DevastatingWave via collision |

## Key Points
- Extends `AGeoDeployableBase` with `bUseRegularDrain = true` (but `LifeDrainMaxDuration = 0` so no drain in practice) — stays alive until damaged to zero or recalled
- When health hits zero, `OnHealthChanged_Implementation` calls `Recall()` — this is the only destruction path
- Overrides `RecallEffect(Value)`: server calls `Explode(Value)` to damage nearby enemies before the deployable expires
- BP overrides `RecallEffect` for death VFX/SFX, then calls Super to trigger the explode
- `FDeployableData` used directly (no extra fields for now)
- Collision profile inherited from base (`GeoCapsule`); BP sets mesh and capsule size via `Data.Params.Size`
