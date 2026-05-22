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
| `AddFireAbilityStateByTagName` | Adds a state with a fire-projectile task; `ParentStateName=None` for root, `InsertIndex=-1` to append |
| `ReplaceFireAbilityTagInState` | Finds a state by name, replaces its task tag, compiles, saves |
| `RemoveState` | Removes a state by name (recursive), compiles, saves |
| `ClearTransitions` | Removes all transitions from a state, compiles, saves |
| `AddTransition` | Adds a GotoState transition with a trigger enum, compiles, saves |
