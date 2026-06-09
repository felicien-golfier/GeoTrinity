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

**Call from Python** — get the CDO via `get_default_object()`, then call the method. Python snake_cases UFUNCTION names automatically. Pass `FGameplayTag` args via `import_text`. See `AI/Python/` for call patterns.

**Confirm a new signature is loaded first** — after a rebuild that adds a UFUNCTION, parameter, or UENUM, check the type is present on the `unreal` module (e.g. the enum exists, the new attribute resolves) before invoking it; the running editor only sees the new signature once the rebuilt module is loaded.

**Generic, parameterized functions — never one-offs.** A shim function takes arguments and works for any caller; a per-asset hardcoded function is wrong. Before adding one, check the existing utility for a function that already does the operation and extend it (add a parameter) rather than adding a near-duplicate. Compose complex trees by calling several generic primitives with arguments from the Python caller, and share setup/teardown through private helpers.

## Existing Utilities

| Class | Header | What it does |
|---|---|---|
| `UGeoStateTreeBuilderUtil` | `Public/Tool/GeoStateTreeBuilderUtil.h` | Add/remove states, manage transitions on `UStateTree` assets |
| `UGeoWidgetBuilderUtil` | `Public/Tool/GeoWidgetBuilderUtil.h` | Generic widget-tree primitives + inspect on `UWidgetBlueprint` assets (see `MCP_UI.md`) |
| `UGeoHudWidgetBuilderUtil` | `Public/Tool/GeoHudWidgetBuilderUtil.h` | Content-specific widget trees composed from the generic primitives (see `MCP_UI.md`) |
