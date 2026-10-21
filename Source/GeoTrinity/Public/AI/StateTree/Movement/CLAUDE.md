# AI/StateTree/Movement

| File | Role |
|---|---|
| `STTask_MoveTo.h` | Replaces built-in Move To; replans around dynamic nav obstacles; use in `ST_EnemyBehaviour` |
| `GeoAITask_MoveTo.h` | Helper task: auto-repath on invalidation, move cue, face along velocity |
| `STTask_ChaseTarget.h` | Straight-line chase ignoring navmesh (hex boss); never Succeeds |
| `STTask_FaceTarget.h` | Track `GetCurrentTarget()` while rooted; never Succeeds |
| `STTask_MoveToTargetPoint.h` | Straight walk to a random point of a purpose tag in the boss's arena, ignoring the navmesh; Succeeds on arrival (tutorial miniboss) |
| `STTask_SelectNextFiringPoint.h` | Random `AI.FiringPoint` excluding the last, from the boss's own arena (`GetArenaOfBoss`) |

## GeoAITask_MoveTo
- `PerformMove()` calls `Path->EnableRecalculationOnInvalidation(true)`.
- Move cue (`MoveGameplayCueTag`) added on the pawn ASC in `PerformMove`, removed in `OnDestroy`; `RawMagnitude` = path length. `AddGameplayCue` replicates (works on Minimal ASCs) and never dedups — guard with `MoveCueASC`.
- Ticks (`bTickingTask = true`): faces along velocity via `SetTargetYaw`. Logic that must survive replans uses `TickTask`, never world timers.
- Gate paired add/remove on the same condition after `Super::PerformMove()` (it can destroy the task).

## Chase / Face
- Target = `AGeoEnemyAIController::GetCurrentTarget()` (cast the schema's `AAIController` first); stand still while `Running` when nobody lives.
- Facing = `SetTargetYaw`; chase uses `AddMovementInput` while 2D distance > `StopDistance`; turn clamp lives in `AGeoCharacter::Tick`.
- End with transitions; wake with `AI.Boss.AggroEvent`.

## SelectNextFiringPoint
- Instance `TargetLocation` (out); `LastFiringPointIndex` in the blackboard; completes in `EnterState`.

## MoveToTargetPoint
- For a room with no navmesh (`MoveTo` needs one). `EnterState` picks, at random, a `PointTag` point of the pawn's arena (`GetArenaOfBoss`, `ArenaTag`) the pawn is not already at (2D, within `AcceptanceRadius`); none left = `Failed` with a log. `Tick` faces it (`SetTargetYaw`), `AddMovementInput`s straight at it, and Succeeds once within `AcceptanceRadius`.
- The pick lives in the instance data: each entry of the state walks to a new point.
