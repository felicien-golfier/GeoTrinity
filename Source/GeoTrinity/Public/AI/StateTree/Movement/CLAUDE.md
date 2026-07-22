# AI/StateTree/Movement

StateTree tasks that control enemy movement and position selection.

## Files
### `STTask_MoveTo`
Replacement for the built-in `Move To` StateTree task. Replans around dynamic nav obstacles (e.g. pillars) that appear after the initial path is computed.
- Overrides `PrepareMoveToTask` to spawn `UGeoAITask_MoveTo` instead of `UAITask_MoveTo`
- **Always use this task instead of the built-in `Move To` in `ST_EnemyBehaviour`**

### `GeoAITask_MoveTo`
Helper AI task spawned by `STTask_MoveTo`. Overrides `PerformMove()` to call `Path->EnableRecalculationOnInvalidation(true)`, letting the nav system auto-repath when tiles are rebuilt.
- Adds `MoveGameplayCueTag` on the pawn's ASC in `PerformMove`, removes it in `OnDestroy`, so the cue spans the move and ends on arrival or cancel. `RawMagnitude` carries path length for pitch/duration scaling.
- **The cue is how AI cosmetics reach clients** — StateTree AI is server-only, so direct sound calls here are host-only. `AddGameplayCue` replicates unconditionally (`COND_None`) even on Minimal-mode ASCs, and (unlike `ExecuteGameplayCue`) carries full `FGameplayCueParameters` including `RawMagnitude`.
- **`AddGameplayCue` never dedups** — guard every add (`MoveCueASC`), or a replan mid-move stacks a second looping cue.
- **`Super::PerformMove()` can destroy the task synchronously** (on `Failed`/`AlreadyAtGoal`, via `FinishMoveTask`→`OnDestroy`, which nulls `Path`). Any `if (Path.IsValid())` block after `Super::PerformMove()` silently no-ops — gate the paired add/remove logic on the same condition or the cue leaks.
- **Never drive logic off a world timer here** — `UAITask_MoveTo::ResetTimers()` calls `ClearAllTimersForObject(this)` on every replan/Pause/OnDestroy, killing subclass timers too (without invalidating the handle). Use `TickTask` instead (opt in; base class doesn't tick).
- Opts into `TickTask` (`bTickingTask = true` in the constructor) to face the pawn along its current velocity via `AGeoCharacter::SetTargetYaw()` every frame — same facing mechanism as `STTask_ChaseTarget`.

### `STTask_ChaseTarget`
Straight-line chase for bosses that ignore navmesh and terrain holes (hex arena boss). Ticks every frame:
- Reads `AGeoEnemyAIController::GetCurrentTarget()` — resolved once per frame on the controller (see
  `AI/CLAUDE.md`), not re-resolved per task — stands still (still `Running`) when nobody is alive
- Sets the pawn's facing via `AGeoCharacter::SetTargetYaw()` — **does not use `AIController::SetFocus`**, since
  `UpdateControlRotation` snaps `SetControlRotation` straight to the focal point every frame, bypassing any turn-rate
  clamp — and `AddMovementInput` while the 2D distance exceeds `StopDistance`; speed comes from the pawn's movement
  component. The actual turn-rate clamp lives in `AGeoCharacter::Tick` (`MaxRotationSpeed`), shared with
  `APlayableCharacter::UpdateAimRotation` and `GeoAITask_MoveTo::TickTask`.
- **`InstanceData.AIController` is typed `AAIController`** (the schema's context type) — cast to
  `AGeoEnemyAIController` before calling `GetCurrentTarget()`.
- Never returns Succeeded — end it with transitions. Wake the tree with `AI.Boss.AggroEvent` (sent by
  `AGeoEnemyAIController::TriggerAggro` for non-match bosses)

### `STTask_FaceTarget`
Same target read (`AGeoEnemyAIController::GetCurrentTarget()`) and `SetTargetYaw()` facing as `STTask_ChaseTarget`,
minus the movement — for bosses that must track a target while rooted (e.g. a channeled attack). Never returns
Succeeded; end it with transitions.

### `STTask_SelectNextFiringPoint`
Picks a random firing point (tagged `AI.FiringPoint`) excluding the last one used.
- `FInstanceDataType`: `TargetLocation` (output `FVector`)
- Links `UGeoAIBlackboardComponent` via `BlackboardHandle` — reads/writes `LastFiringPointIndex`
- `EnterState` — picks a random valid point (excluding last), writes location into `TargetLocation`, completes immediately
