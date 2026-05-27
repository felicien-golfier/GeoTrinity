# AI/StateTree/Movement

StateTree tasks that control enemy movement and position selection.

## Files
### `STTask_MoveTo`
Replacement for the built-in `Move To` StateTree task. Replans around dynamic nav obstacles (e.g. pillars) that appear after the initial path is computed.
- Overrides `PrepareMoveToTask` to spawn `UGeoAITask_MoveTo` instead of `UAITask_MoveTo`
- **Always use this task instead of the built-in `Move To` in `ST_EnemyBehaviour`**

### `GeoAITask_MoveTo`
Helper AI task spawned by `STTask_MoveTo`. Overrides `PerformMove()` to call `Path->EnableRecalculationOnInvalidation(true)` after `Super`, enabling the nav system to auto-repath when tiles are rebuilt.

### `STTask_SelectNextFiringPoint`
Picks a random firing point (tagged `AI.FiringPoint`) excluding the last one used.
- `FInstanceDataType`: `TargetLocation` (output `FVector`)
- Links `UGeoAIBlackboardComponent` via `BlackboardHandle` — reads/writes `LastFiringPointIndex`
- `EnterState` — picks a random valid point (excluding last), writes location into `TargetLocation`, completes immediately
