# MCP Editor Utility Objects

Pattern for exposing C++ editor operations to Python when `get/set_editor_property` can't reach them (protected properties, template APIs, editor-only subsystems).

## When to Use

- Property is protected or not UPROPERTY-exposed (e.g. `EditorData` on `UStateTree`)
- Need to call a C++ template function (`AddTask<T>`, `AddEvaluator<T>`, etc.)
- Need an editor subsystem with no Python binding

## Pattern

**Header** — `Source/GeoTrinity/Public/Tool/GeoXxxUtil.h`
- Extend `UEditorUtilityObject`, wrap in `#if WITH_EDITOR`
- All methods `static`, decorated `UFUNCTION(BlueprintCallable, CallInEditor)`

**Implementation** — `Source/GeoTrinity/Private/Tool/GeoXxxUtil.cpp`
- Save assets with `UEditorLoadingAndSavingUtils::SavePackages({ Asset->GetPackage() }, false)` from `FileHelpers.h`

**Build.cs** — add editor-only deps inside `if (Target.bBuildEditor)`:
- `"UnrealEd"` — for `UEditorLoadingAndSavingUtils`, `GEditor`
- `"Blutility"` — for `UEditorUtilityObject`
- After any Build.cs change: **close editor and do a full build** before Live Coding.

**Call from Python**
```python
cdo = unreal.GeoXxxUtil.get_default_object()
cdo.do_something(arg1, arg2)
```
Python snake_cases UFUNCTION names automatically. Pass `FGameplayTag` args via `import_text`.

## Existing Utilities

| Class | Header | What it does |
|---|---|---|
| `UGeoStateTreeBuilderUtil` | `Public/Tool/GeoStateTreeBuilderUtil.h` | Add/remove states, manage transitions on `UStateTree` assets |
