# AI/StateTree

StateTree task implementations for enemy AI. Add all new AI behavior here.

## Task Authoring Pattern
Tasks are `USTRUCT` extending `FStateTreeTaskCommonBase` or `FStateTreeAIActionTaskBase`.

**Required:**
- Separate `FInstanceDataType` struct for per-instance data (inputs/outputs/state)
- Override `GetInstanceDataType()`: `return FInstanceDataType::StaticStruct();`
- Access owner: `Context.GetOwner()` — returns AIController or Pawn depending on schema

**Always check Epic source before implementing**: `FStateTreeDelayTask`, `FStateTreeMoveToTask` in `Engine\Plugins\Runtime\GameplayStateTree\Source\`. Method names change between engine versions.

## Completion Patterns

**Async (preferred — delegate-based tasks):**
```cpp
// Constructor
bShouldCallTick = false;

// EnterState
auto WeakContext = Context.MakeWeakExecutionContext();
SomeDelegate.BindLambda([WeakContext]() {
    WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
});
```
Include: `StateTreeAsyncExecutionContext.h`

**Scheduled tick (timer-based tasks):**
```cpp
Context.UpdateScheduledTickRequest(Handle, FStateTreeScheduledTick::MakeCustomTickRate(TimeSeconds));
```
Only ticks when needed — not every frame.

## Existing Tasks

### `STTask_FireProjectileAbility`
Activates an ability by `GameplayTag` on the enemy's ASC.
- `FInstanceDataType`: `AbilityTag` (input), `AbilityEndedDelegateHandle` (cleanup)
- `EnterState` — finds ASC, activates ability, binds to ability-ended delegate for async completion
- `ExitState` — unbinds delegate

### `STTask_MoveTo`
Replacement for the built-in `Move To` StateTree task. Replans around dynamic nav obstacles (e.g. pillars) that appear after the initial path is computed.
- Overrides `PrepareMoveToTask` to spawn `UGeoAITask_MoveTo` instead of `UAITask_MoveTo`
- `UGeoAITask_MoveTo` overrides `PerformMove()` to call `Path->EnableRecalculationOnInvalidation(true)` after `Super`, enabling the nav system to auto-repath when tiles are rebuilt
- **Always use this task instead of the built-in `Move To` in `ST_EnemyBehaviour`**

### `STTask_SelectNextFiringPoint`
Round-robin firing point selection from `AEnemyCharacter::FiringPoints`.
- `FInstanceDataType`: `TargetLocation` (output `FVector`)
- `EnterState` — calls `GetAndAdvanceNextFiringPointLocation()`, writes to `TargetLocation`, completes immediately

---

## Property Functions

Property functions are `USTRUCT` extending `FStateTreePropertyFunctionCommonBase`. They run each frame to compute a value and write it to an output property, which can then be bound to any condition or task input.

### `FSTGetHealthRatioPropertyFunction`
Reads the controlled pawn's current health ratio from `UGeoAttributeSetBase`.
- `Input` — `AAIController*` (bind to the owning controller context)
- `Output` — `float` in [0, 1]; outputs 0 if the controller, pawn, or ASC is absent
