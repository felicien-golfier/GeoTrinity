# MCP StateTree Editing

`UStateTree::EditorData` is protected — Python can't touch it. Use the C++ `UEditorUtilityObject` shim `UGeoStateTreeBuilderUtil`.

## Key Constraints

- Take `FName` for tags in UFUNCTIONs — convert with `FGameplayTag::RequestGameplayTag` inside C++.
- Guard `GetPtr<T>` calls with a struct type check before accessing — assert fires on mismatch.
- States are nested: top-level in `EditorData->SubTrees`, children in each state's `Children` array.
- Adding a new `UFUNCTION` requires a full build. Implementation-only changes can use Live Coding.

## Calling from Python

See `AI/Python/state_tree_edit.py` for all operations with examples.

## Adding New UFUNCTIONs to the Shim

Make UFUNCTIONs as generic as possible — accept struct type, state name, parent, etc. as parameters rather than hardcoding specifics. A parameterized function that works for any struct type is always preferred over a specialized helper.

## Existing Utility

`Source/GeoTrinity/Public/Tool/GeoStateTreeBuilderUtil.h`

| Method | What it does |
|---|---|
| `ListStates` | Logs all states recursively with indent, task tags, and transitions |
| `AddState` | Adds an empty state with no task; for idle states gated by an event transition |
| `AddTaskToState` | Adds a task of any struct type (by unqualified name) to an existing state with default instance data |
| `AddFireAbilityStateByTagName` | Adds a state with a fire-projectile task; pass `"None"` (string) for root parent, `InsertIndex=-1` to append |
| `ReplaceFireAbilityTagInState` | Finds a state by name, replaces its task tag, compiles, saves |
| `AddFloatEnterCondition` | Appends a `Float Compare` enter condition to a state; sets `Right` (threshold) and operator |
| `BindConditionPropertyToPropertyFunction` | Binds any condition property to a Property Function output; also binds the function's Input to a context class — use this after `AddFloatEnterCondition` to wire `Left` |
| `RemoveState` | Removes a state by name (recursive), compiles, saves |
| `ClearTransitions` | Removes all transitions from a state, compiles, saves |
| `AddTransition` | Adds a GotoState transition with a trigger enum; pass an event tag name for `OnEvent` triggers, compiles, saves |

## Enter Conditions

Enter conditions gate whether a state can be entered on each tick — they are evaluated before the state activates.

- `FStateTreeCompareFloatCondition` is the standard struct for numeric comparisons; use `AddFloatEnterCondition` to append one via Python.
- Call `BindConditionPropertyToPropertyFunction` immediately after `AddFloatEnterCondition` to wire unbound properties.
- `BindConditionPropertyToPropertyFunction` takes the condition property name, the Property Function struct name, the function's output and input property names, and the context class name — all as `FName`; nothing is hardcoded.

## StateTree Property Functions

Property Functions appear under "Property Functions" in the binding picker and are evaluated each tick to produce a typed output — they are the correct pattern for derived values like health ratio.

- A Property Function is a pair of USTRUCTs: an `InstanceData` struct (holds `Input` / `Output` fields) and the function struct itself inheriting `FStateTreePropertyFunctionCommonBase`.
- The `Input` field drives what object the function reads from — type it as the most specific context class available (e.g. `AAIController*`), not `AActor*`; the `Output` field is what gets bound in the editor.
- Implement `Execute()` to read live data (e.g. via `GeoASLib::GetGeoAscFromActor`) and write to `InstanceData.Output`.
- New USTRUCT files require a full build (UHT must run) — live compile cannot handle them.
- To add a new Property Function for a different attribute, copy `STPropertyFunction_GetHealthRatio.h/.cpp` and change the attribute read in `Execute()`.
- See `Source/GeoTrinity/Public/AI/StateTree/STPropertyFunction_GetHealthRatio.h` as the reference implementation.
