# AI/StateTree/Movement

StateTree tasks that control enemy movement and position selection.

## Files
### `STTask_MoveTo`
Replacement for the built-in `Move To` StateTree task. Replans around dynamic nav obstacles (e.g. pillars) that appear after the initial path is computed.
- Overrides `PrepareMoveToTask` to spawn `UGeoAITask_MoveTo` instead of `UAITask_MoveTo`
- **Always use this task instead of the built-in `Move To` in `ST_EnemyBehaviour`**

### `GeoAITask_MoveTo`
Helper AI task spawned by `STTask_MoveTo`. Overrides `PerformMove()` to call `Path->EnableRecalculationOnInvalidation(true)` after `Super`, enabling the nav system to auto-repath when tiles are rebuilt.
- Also adds `MoveGameplayCueTag` on the pawn's ASC in `PerformMove` and removes it in `OnDestroy`, so the cue spans the
  move and ends on arrival *and* on interruption/cancel. `RawMagnitude` carries the path length; the notify owns the
  sound/VFX and any pitch or duration scaling.
- **The cue is how AI cosmetics reach clients.** StateTree AI runs on the server only — clients have no move task, so
  `UGameplayStatics::SpawnSoundAttached`/`PlaySoundAtLocation` here are audible on the listen host and nowhere else.
  `AddGameplayCue` writes `ActiveGameplayCues`, which replicates unconditionally (`COND_None`) regardless of the ASC's
  replication mode, so Minimal-mode enemy ASCs are fine. Unlike `ExecuteGameplayCue`'s context-only multicast, the
  `AddGameplayCue` path sends full `FGameplayCueParameters` — `RawMagnitude` survives to `OnActive` and `WhileActive`
  (it is only serialized when non-zero).
- **`AddGameplayCue` never dedups** — `FActiveGameplayCueContainer::AddCue` appends unconditionally, and `PerformMove`
  re-enters on every replan. Guard every add (`MoveCueASC`), or replans stack a second looping cue.
- **`Super::PerformMove()` can destroy the task before it returns.** `UAITask_MoveTo::PerformMove` calls
  `AAIController::MoveTo`, and on `Failed` (goal won't project to the navmesh — e.g. a pillar carved it out) or
  `AlreadyAtGoal` (`PathFollowingComponent::HasReached`) it runs `FinishMoveTask` → `EndTask` → `OnDestroy`
  *synchronously, inside the `Super` call*. `OnDestroy` then nulls `Path`, so any `if (Path.IsValid())` block after
  `Super::PerformMove()` is silently skipped. Anything paired with such a block (adding a cue vs. removing it) must
  be gated on the same condition, or the end half fires for moves that never happened.
- **Never drive anything from a world timer in a `UAITask_MoveTo` subclass.** `UAITask_MoveTo::ResetTimers()` calls `ClearAllTimersForObject(this)`, which kills *every* timer bound to the task — including ones a subclass registered. It runs at the top of every `PerformMove()` (so on each replan), plus `Pause()` and `OnDestroy()`. It also does not invalidate your `FTimerHandle`, so the handle keeps reading as valid while the timer is already gone. Use `TickTask` instead (the base class neither ticks nor sets `bTickingTask` — a subclass must opt in).

### `STTask_SelectNextFiringPoint`
Picks a random firing point (tagged `AI.FiringPoint`) excluding the last one used.
- `FInstanceDataType`: `TargetLocation` (output `FVector`)
- Links `UGeoAIBlackboardComponent` via `BlackboardHandle` — reads/writes `LastFiringPointIndex`
- `EnterState` — picks a random valid point (excluding last), writes location into `TargetLocation`, completes immediately
