# Actor/Deployable/BombZone

Hex-boss tile bomb deployable. Dropped on and attached to a player by `UGeoTileBombAbility`.

## Files
| File | Role |
|---|---|
| `GeoBombZone.h/.cpp` | Two-phase bomb — rides the carrier, then plants and carves the tiles under it |

## Key Points
- **The drain/blink lifecycle *is* the two phases.** `bUseRegularDrain` + `bAutoRecallAtEndLife` (like `AGeoMine`): the drain (`LifeDrainMaxDuration`) runs Phase A while the bomb is attached to the carrier; the blink (`BlinkDuration`) is Phase B, the planted telegraph. Total fuse = `LifeDrainMaxDuration + BlinkDuration`.
- **Attachment is the "ride the carrier" mechanism**, not a stored pointer. The ability `AttachToActor`s the bomb to the carrier on the server; the replicated attachment tracks the authoritative carrier position everywhere, so no client-position RPC is needed.
- **Detach at blink start plants the bomb.** `OnBlinkVisualStarted` (runs on every machine) `DetachFromActor(KeepWorld)` **on the server only** — the frozen world position then replicates down. This is what turns "follows you" into "plant-and-flee": move the detach to `RecallEffect` instead and it would instead blow up wherever you end up.
- `RecallEffect` (server, `bExplodeAtRecall = true`) runs the base `Explode` first (effect data within `Params.Size`, damage before tiles), then `DestroyTilesInRadius` at the same spot. Unlike `AGeoMine` it does **not** gate on `IsOverAliveTile` — carving is the bomb's whole point, so a tile already gone under it (e.g. another boss ability) just widens the hole.
- **Not damageable**, `NoCollision` — a pure indicator riding the carrier; the drain (GAS) drives the fuse regardless. `bDestroyOldestWhenLimitReached` keeps the spawn from ever being blocked by the boss's deployable cap, so the ability needs no `SetDeployableInfinitCount`.
- `BombData` replicated `COND_InitialOnly`, like `AGeoMine`/`AGeoPillar`.
- **BP visual = the telegraph.** Author a ground decal/ring on the BP sized to `Params.Size` (read from `GetData()`); attachment makes it follow in Phase A and the detach freezes it for Phase B. The base blink flashing adds the "about to blow" urgency.
