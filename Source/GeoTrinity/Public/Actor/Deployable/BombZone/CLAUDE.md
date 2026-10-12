# Actor/Deployable/BombZone

`GeoBombZone.h` — hex-boss tile bomb, dropped on and attached to a player by `UGeoTileBombAbility`.

- The drain/blink lifecycle is the two phases: drain (`LifeDrainMaxDuration`) = riding the carrier, blink (`BlinkDuration`) = planted telegraph. Fuse = sum.
- Ride = server `AttachToActor` by the ability; replicated attachment tracks the carrier everywhere.
- `OnBlinkVisualStarted` detaches (`KeepWorld`) on the server only → plant-and-flee.
- `RecallEffect` (server, `bExplodeAtRecall`): base `Explode` first, then `DestroyTilesInRadius`; no `IsOverAliveTile` gate (a gone tile just widens the hole).
- Not damageable, `NoCollision`; `bDestroyOldestWhenLimitReached` keeps spawn unblocked.
- BP visual = ground decal/ring sized to `Params.Size` (from `GetData()`); `BombData` `COND_InitialOnly`.
