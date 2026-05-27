# AI/StateTree

StateTree task implementations for enemy AI. Add all new AI behavior here.

## Folder Layout
| Folder | Contents |
|---|---|
| `Ability/` | `STTask_FireAbility` — see `Ability/CLAUDE.md` |
| `Blackboard/` | `STTask_SetBlackboard` — see `Blackboard/CLAUDE.md` |
| `Movement/` | `STTask_MoveTo`, `GeoAITask_MoveTo`, `STTask_SelectNextFiringPoint` — see `Movement/CLAUDE.md` |
| `Property/` | `STPropertyFunction_GetHealthRatio`, `STPropertyFunction_GetBlackboard` — see `Property/CLAUDE.md` |
| `Utility/` | `STTask_SendEventAfterNCycles` — see `Utility/CLAUDE.md` |

## Task Authoring Pattern
Tasks are `USTRUCT` extending `FStateTreeTaskCommonBase` or `FStateTreeAIActionTaskBase`.

**Required:**
- Separate `FInstanceDataType` struct for per-instance data (inputs/outputs/state)
- Override `GetInstanceDataType()`: `return FInstanceDataType::StaticStruct();`
- Access owner: `Context.GetOwner()` — returns AIController or Pawn depending on schema

**Always check Epic source before implementing**: `FStateTreeDelayTask`, `FStateTreeMoveToTask` in `Engine\Plugins\Runtime\GameplayStateTree\Source\`. Method names change between engine versions.

## Completion Patterns

**Async (delegate-based):** set `bShouldCallTick = false` in the constructor, capture a `MakeWeakExecutionContext()` in `EnterState`, and call `FinishTask` from the delegate. Include `StateTreeAsyncExecutionContext.h`. See `STTask_FireAbility` for a concrete example.

**Scheduled tick (timer-based):** call `Context.UpdateScheduledTickRequest` with `FStateTreeScheduledTick::MakeCustomTickRate` — only ticks when needed, not every frame.
