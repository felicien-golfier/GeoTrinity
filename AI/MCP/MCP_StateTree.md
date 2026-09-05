# MCP StateTree Editing

`UStateTree::EditorData` is protected, so every edit goes through the `UGeoStateTreeBuilderUtil` shim
(`Source/GeoTrinityEditor/Public/Tool/GeoStateTreeBuilderUtil.h`). See `AI/Python/Ability/state_tree_edit.py` for all
operations with examples, and `MCP_EditorUtility.md` for the shim pattern and its generic-function rule.

## Constraints

- States are nested: top-level in `EditorData->SubTrees`, children in each state's `Children` array.
- Take `FName` for tags in UFUNCTIONs and convert inside C++.
- Guard `GetPtr<T>` with a struct type check — the assert fires on a mismatch.
- A new `UFUNCTION` or a new USTRUCT needs a full build (UHT must run); implementation-only changes can use
  Live Coding.

## Shim methods

| Method | What it does |
|---|---|
| `ListStates` | Logs all states recursively with indent, task tags and transitions |
| `AddState` | Adds an empty state with no task, for an idle state gated by an event transition |
| `RemoveState` | Removes a state by name, recursively |
| `AddTaskToState` | Adds a task of any struct type (by unqualified name) with default instance data |
| `AddFireAbilityStateByTagName` | Adds a state with a fire-projectile task; `"None"` for the root parent, `InsertIndex=-1` to append |
| `AddFireAbilityTaskToState` | Adds one fire-ability task to an existing state; several in one state fire together |
| `ReplaceFireAbilityTagInState` | Finds a state by name and replaces its task tag |
| `SetTasksCompletion` | Whether a state waits for all its tasks or completes on the first |
| `ClearTransitions` / `AddTransition` | Removes all transitions; adds a GotoState transition with a trigger enum (an event tag name for `OnEvent`) |
| `AddFloatEnterCondition` | Appends a `Float Compare` enter condition, setting the threshold and operator |
| `BindConditionPropertyToPropertyFunction` | Binds any condition property to a Property Function output and the function's input to a context class |

## Enter conditions

Enter conditions gate whether a state can be entered, evaluated each tick before the state activates.
`FStateTreeCompareFloatCondition` is the standard numeric one; call the binding method immediately after adding
it to wire the unbound left-hand property. The binding takes a struct naming the condition property, the
Property Function struct, its output and input properties, and the context class — nothing is hardcoded.

## Property Functions

A Property Function appears under "Property Functions" in the binding picker and is evaluated each tick to
produce a typed output, which is the correct pattern for a derived value like a health ratio.

It is a pair of USTRUCTs: an instance-data struct holding `Input` / `Output` fields, and the function struct
inheriting `FStateTreePropertyFunctionCommonBase`. Type `Input` as the most specific context class available
rather than an actor, since it drives what object the function reads from; `Output` is what gets bound in the
editor. `Execute()` reads the live data and writes to the output.

Adding one for a different attribute is a copy of the reference implementation with a different attribute read
in `Execute()` — see `Source/GeoTrinity/Public/AI/StateTree/STPropertyFunction_GetHealthRatio.h`.
