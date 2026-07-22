# Actor/Deployable/Pillar

Boss-fight pillar deployable. Spawned by `GeoFatalZone` when its countdown expires.

## Files
| File | Role |
|---|---|
| `GeoPillar.h/.cpp` | Deployable pillar — has health, no drain, blocks DevastatingWave via collision |

## Key Points
- `bUseRegularDrain = true` but `LifeDrainMaxDuration = 0` — no real drain; stays alive until damaged to zero or recalled.
- `bPushActorsOnSpawn = true` — pushes overlapping characters outward on spawn (base `PushAway()`).
- Zero health → `OnHealthChanged_Implementation` calls `Recall()` (only destruction path). `RecallEffect(Value)` calls `Explode(Value)` before expiring; BP overrides `RecallEffect` for VFX/SFX then must call Super to trigger the explode.
- `PillarData` replicated `COND_InitialOnly`.
