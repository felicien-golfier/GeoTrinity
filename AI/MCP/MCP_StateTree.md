# MCP StateTree Editing

`UStateTree::EditorData` is protected — Python can't touch it. Use the C++ `UEditorUtilityObject` shim `UGeoStateTreeBuilderUtil`.

## Key Constraints

- Take `FName` for tags in UFUNCTIONs — convert with `FGameplayTag::RequestGameplayTag(Name, false)` inside C++.
- `GetPtr<T>` on `FStateTreeEditorNode::GetInstance()` asserts if the struct type doesn't match — always guard with `GetStruct()->IsChildOf(T::StaticStruct())` first.
- States are nested: top-level in `EditorData->SubTrees`, children in `UStateTreeState::Children`.
- Adding a new `UFUNCTION` requires a full build. Implementation-only changes can use Live Coding.

## Calling from Python

See `AI/Python/state_tree_edit.py` for all operations with examples.

```python
cdo = unreal.GeoStateTreeBuilderUtil.get_default_object()
st  = unreal.load_asset("/Game/AI/ST_EnemyBehaviour")
cdo.list_states(st)
```

## Existing Utility

`Source/GeoTrinity/Public/Tool/GeoStateTreeBuilderUtil.h`

| Method | What it does |
|---|---|
| `ListStates` | Logs all states recursively with indent, task tags, and transitions |
| `AddFireAbilityStateByTagName` | Adds a state with `FSTTask_FireProjectileAbility`; `ParentStateName=None` for root, `InsertIndex=-1` to append |
| `ReplaceFireAbilityTagInState` | Finds a state by name, replaces its task tag, compiles, saves |
| `RemoveState` | Removes a state by name (recursive), compiles, saves |
| `ClearTransitions` | Removes all transitions from a state, compiles, saves |
| `AddTransition` | Adds a GotoState transition with `EStateTreeTransitionTrigger`, compiles, saves |
