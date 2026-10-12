# Actor/Deployable/Pillar

`GeoPillar.h` — boss pillar spawned by `USpawnPillarPattern`; blocks the devastating wave by collision.

- `bUseRegularDrain` with `LifeDrainMaxDuration = 0`: lives until damaged to zero or recalled.
- `bPushActorsOnSpawn` (base `PushAway()`).
- Zero health → `Recall()` (only destruction path); `RecallEffect` calls `Explode(Value)`; BP overrides must call Super.
- `PillarData` `COND_InitialOnly`.
