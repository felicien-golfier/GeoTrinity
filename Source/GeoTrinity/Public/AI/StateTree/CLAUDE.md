# AI/StateTree

All new AI behaviour goes here. Editor-side editing: `AI/MCP/MCP_StateTree.md`, builder `GeoStateTreeBuilderUtil`.

| Folder | Holds |
|---|---|
| `Ability/` | `STTask_FireAbility` |
| `Blackboard/` | `STTask_UpdateBlackboard` |
| `Movement/` | `STTask_MoveTo`, `GeoAITask_MoveTo`, `STTask_ChaseTarget`, `STTask_FaceTarget`, `STTask_SelectNextFiringPoint` |
| `Property/` | `STPropertyFunction_GetHealthRatio`, `STPropertyFunction_GetBlackboard` |
| `Utility/` | `STTask_SendEventAfterNCycles`, `STTask_PlayMontage`, `STTask_SetInvulnerable` |

## Task pattern
- `USTRUCT` extending `FStateTreeTaskCommonBase` / `FStateTreeAIActionTaskBase`.
- Separate `FInstanceDataType` + `GetInstanceDataType()`; owner via `Context.GetOwner()`.
- Check the engine's `FStateTreeDelayTask` / `FStateTreeMoveToTask` first (`Engine\Plugins\Runtime\GameplayStateTree`).
- Async completion: `bShouldCallTick = false`, `MakeWeakExecutionContext()` in `EnterState`, `FinishTask` from the delegate (include `StateTreeAsyncExecutionContext.h`; example `STTask_FireAbility`).
- Timer completion: `Context.UpdateScheduledTickRequest` + `FStateTreeScheduledTick::MakeCustomTickRate`.
- Durable cross-state data goes in `FGeoAIBlackboardData`, not instance data.
