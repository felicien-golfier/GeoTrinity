# AI/StateTree/Utility

General-purpose StateTree tasks that don't fit movement, ability, or blackboard categories.

## Files
### `STTask_SendEventAfterNCycles`
Counts state entries and sends a GameplayTag event when the count reaches the threshold.
- `FInstanceDataType`: `CyclesRequired` (input `int32`), `EventTag` (input `FGameplayTag`)
- Links `UGeoAIBlackboardComponent` via `BlackboardHandle` — reads/writes `CycleCount`
- `EnterState` — increments counter; sends event and resets to 0 when `CycleCount >= CyclesRequired`
