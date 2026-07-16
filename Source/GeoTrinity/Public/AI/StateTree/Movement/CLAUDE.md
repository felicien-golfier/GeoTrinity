# AI/StateTree/Movement

StateTree tasks that control enemy movement and position selection.

## Files
### `STTask_MoveTo`
Replacement for the built-in `Move To` StateTree task. Replans around dynamic nav obstacles (e.g. pillars) that appear after the initial path is computed.
- Overrides `PrepareMoveToTask` to spawn `UGeoAITask_MoveTo` instead of `UAITask_MoveTo`
- **Always use this task instead of the built-in `Move To` in `ST_EnemyBehaviour`**

### `GeoAITask_MoveTo`
Helper AI task spawned by `STTask_MoveTo`. Overrides `PerformMove()` to call `Path->EnableRecalculationOnInvalidation(true)` after `Super`, enabling the nav system to auto-repath when tiles are rebuilt.
- Also plays `MoveSound` as a loop on the pawn and drives its pitch from `PitchCurve` over the path's travel time (`length / max speed`), updated from `TickTask` (`bTickingTask = true`, set in the constructor).
- Plays `EndSound` once at the pawn's location from `OnDestroy`, so it fires on arrival *and* on interruption/cancel.
- **Never drive anything from a world timer in a `UAITask_MoveTo` subclass.** `UAITask_MoveTo::ResetTimers()` calls `ClearAllTimersForObject(this)`, which kills *every* timer bound to the task — including ones a subclass registered. It runs at the top of every `PerformMove()` (so on each replan), plus `Pause()` and `OnDestroy()`. It also does not invalidate your `FTimerHandle`, so the handle keeps reading as valid while the timer is already gone. Use `TickTask` instead.

### `STTask_SelectNextFiringPoint`
Picks a random firing point (tagged `AI.FiringPoint`) excluding the last one used.
- `FInstanceDataType`: `TargetLocation` (output `FVector`)
- Links `UGeoAIBlackboardComponent` via `BlackboardHandle` — reads/writes `LastFiringPointIndex`
- `EnterState` — picks a random valid point (excluding last), writes location into `TargetLocation`, completes immediately
