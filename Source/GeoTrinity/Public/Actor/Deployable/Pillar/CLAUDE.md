# Actor/Deployable/Pillar

Boss-fight pillar deployable. Spawned by `GeoFatalZone` when its countdown expires.

## Files
| File | Role |
|---|---|
| `GeoPillar.h/.cpp` | Deployable pillar — has health, no drain, blocks DevastatingWave via collision |

## Key Points
- Extends `AGeoDeployableBase` with `bUseRegularDrain = true` (but `LifeDrainMaxDuration = 0` so no drain in practice) — stays alive until damaged to zero or recalled
- `bPushActorsOnSpawn = true` — on spawn the pillar root-motion-pushes overlapping characters outward before enabling blocking collision (via base-class `PushAway()`)
- When health hits zero, `OnHealthChanged_Implementation` calls `Recall()` — this is the only destruction path
- Overrides `RecallEffect(Value)`: server calls `Explode(Value)` to damage nearby enemies before the deployable expires
- BP overrides `RecallEffect` for death VFX/SFX, then calls Super to trigger the explode
- `PillarData` (`FDeployableData`) is replicated `COND_InitialOnly` so clients receive spawn parameters without continuous replication overhead
- Collision profile inherited from base (`GeoCapsule`); BP sets mesh and capsule size via `Data.Params.Size`
