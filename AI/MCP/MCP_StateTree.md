# MCP StateTree Editing

`UStateTree::EditorData` is protected — Python can't touch it. Use a C++ `UEditorUtilityObject` shim.

---

## Key Constraints

- Take `FName` for tags in UFUNCTIONs — `GameplayTag` is not constructible from Python. Convert with `FGameplayTag::RequestGameplayTag(Name, false)` inside C++.
- `GetPtr<T>` / `GetMutablePtr<T>` on `FStateTreeEditorNode::GetInstance()` **asserts** (not returns null) if the struct type doesn't match — always guard with `GetStruct()->IsChildOf(T::StaticStruct())` first.
- States are nested: top-level in `EditorData->SubTrees`, children in `UStateTreeState::Children`. Always recurse.
- Adding a new `UFUNCTION` requires a full build. Implementation-only changes can use Live Coding.

---

## Calling from Python

```python
cdo = unreal.GeoStateTreeBuilderUtil.get_default_object()  # NOT load_class(...).get_default_object()
st = unreal.load_asset("/Game/AI/ST_EnemyBehaviour")

cdo.list_states(st)                                                          # inspect the tree first
cdo.add_fire_ability_state_by_tag_name(st, "Fire_X", "Ability.Spell.X")     # add new state
cdo.replace_fire_ability_tag_in_state(st, "Attack", "Ability.Spell.X")      # patch existing state
```

---

## Existing Utility

`Source/GeoTrinity/Public/Tool/GeoStateTreeBuilderUtil.h` — see also the `.cpp` for the recursive helper pattern.

| Method | What it does |
|---|---|
| `ListStates` | Logs all states recursively with indent, task tags, and transitions |
| `AddFireAbilityStateByTagName` | Adds a state with `FSTTask_FireProjectileAbility`; pass `ParentStateName`=None for root, `InsertIndex`=-1 to append |
| `ReplaceFireAbilityTagInState` | Finds a state by name (recursive), replaces its task tag, compiles, saves |
| `RemoveState` | Removes a state by name (recursive search), compiles, saves |
| `ClearTransitions` | Removes all transitions from a state, compiles, saves |
| `AddTransition` | Adds a GotoState transition with a given `EStateTreeTransitionTrigger` (OnStateSucceeded / OnStateFailed / OnStateCompleted), compiles, saves |
