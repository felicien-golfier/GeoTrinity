# Actor/Deployable/Pillar

`GeoPillar.h` — boss pillar spawned by `USpawnPillarPattern`; blocks the devastating wave by collision.

- `bUseRegularDrain` with `LifeDrainMaxDuration = 0`: lives until damaged to zero or recalled.
- `bPushActorsOnSpawn` (base `PushAway()`).
- Spawned at the *start* of `USpawnPillarPattern` with `InitDuration` = the pattern's wind-up left: it drops in during the telegraph (base "Init phase", `../CLAUDE.md`) with `bAutoFinishInit` false: the pattern releases it once the hazard is judged, so its own explosion never hits it. Drop look: `InitStartOffset` set in the constructor; `InitCurve`, `InitMontage`, `InitCue` (its footprint telegraph, `Damage`) and the mesh on `BP_Pillar_Deployable`.
- Zero health → `Recall()` (only destruction path); `RecallEffect` calls `Explode(Value)`; BP overrides must call Super.
- `PillarData` `COND_InitialOnly`.
