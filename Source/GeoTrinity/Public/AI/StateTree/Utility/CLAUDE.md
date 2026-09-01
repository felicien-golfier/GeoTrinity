# AI/StateTree/Utility

General-purpose StateTree tasks that don't fit movement, ability, or blackboard categories.

## Files
### `STTask_SendEventAfterNCycles`
Counts state entries and sends a GameplayTag event when the count reaches the threshold.
- `FInstanceDataType`: `CyclesRequired` (input `int32`), `EventTag` (input `FGameplayTag`)
- Links `UGeoAIBlackboardComponent` via `BlackboardHandle` — reads/writes `CycleCount`
- `EnterState` — increments counter; sends event and resets to 0 when `CycleCount >= CyclesRequired`

### `STTask_PlayMontage`
Plays a montage through the pawn's ASC so it replicates to every client — the tree runs server-only, so a bare `Montage_Play` would be invisible to remote clients.
- `FInstanceDataType`: `Montage` (the asset), `PlayRate` (float, clamped ≥ 0.01)
- Async completion: `bShouldCallTick = false`; `EnterState` binds `FOnMontageEnded` via `MakeWeakExecutionContext` and calls `FinishTask` (Failed on interrupt, Succeeded otherwise)
- `ExitState` calls `StopMontageIfCurrent` — safe no-op if the montage already ended naturally
