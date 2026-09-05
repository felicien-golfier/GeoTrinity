# MCP Editor Utility Objects

Pattern for exposing C++ editor operations to Python when the property accessors cannot reach them — a
protected or non-UPROPERTY property, a C++ template function, an editor subsystem with no Python binding.

A route is never the alternative: the Go MCP server forwards only the JSON fields its tool schema names and
silently drops the rest, so a route can carry no argument its schema does not already name. Anything needing
more goes through the shim, never a route operation with the values baked in.

## Pattern

**Header** — `Source/GeoTrinityEditor/Public/Tool/GeoXxxUtil.h`: extend `UEditorUtilityObject`, no
`#if WITH_EDITOR` guard (the whole module is editor-only), all methods `static` and
`UFUNCTION(BlueprintCallable, CallInEditor)`.

**Implementation** — save assets with `UEditorLoadingAndSavingUtils::SavePackages` from `FileHelpers.h`.

**Build.cs** — editor-only deps inside `if (Target.bBuildEditor)`: `"UnrealEd"` for the saving utils and
`GEditor`, `"Blutility"` for `UEditorUtilityObject`, `"UMGEditor"` for widget work. After any Build.cs change,
close the editor and do a full build before Live Coding.

**Call from Python** — get the CDO with `get_default_object()` and call the method; Python snake_cases UFUNCTION
names automatically, and `FGameplayTag` arguments pass through `import_text`. After a rebuild that adds a
UFUNCTION, parameter or UENUM, confirm the new type resolves on the `unreal` module before invoking it — the
running editor sees the new signature only once the rebuilt module is loaded.

## Generic, parameterized functions — never one-offs

A shim function takes arguments and works for any caller; a per-asset hardcoded one is wrong. Before adding a
function, check the existing utility for one that already does the operation and extend it with a parameter
rather than adding a near-duplicate. Compose complex trees by calling several generic primitives with arguments
from the Python caller, and share setup/teardown through private helpers.

## Existing utilities

| Class | Header (`Source/GeoTrinityEditor/Public/Tool/`) | What it does |
|---|---|---|
| `UGeoStateTreeBuilderUtil` | `GeoStateTreeBuilderUtil.h` | Add/remove states, manage transitions on `UStateTree` assets — `MCP_StateTree.md` |
| `UGeoWidgetBuilderUtil` | `GeoWidgetBuilderUtil.h` | Generic widget-tree primitives and inspection on `UWidgetBlueprint` — `MCP_UI.md` |
| `UGeoHudWidgetBuilderUtil` | `GeoHudWidgetBuilderUtil.h` | Content-specific widget trees composed from those primitives — `MCP_UI.md` |
| `UGeoNiagaraBuilderUtil` | `GeoNiagaraBuilderUtil.h` | Emitters, modules, static switches, input values, dynamic inputs, stage dumps — `MCP_Niagara.md` |
| `UGeoAnimBuilderUtil` | `GeoAnimBuilderUtil.h` | Montage slots/sections/links, layout inspection, skeletal mesh vertex reads, rebuilding a skeletal mesh from a static mesh — `MCP_Animation.md` |

## Bridge-side C++ constraints

Set `FSavePackageArgs::SaveFlags` to `SAVE_NoError` when saving a package, or `SerializeLocMetadataValue`
crashes.

`FTopLevelAssetPath` asserts on a short class name, so a class filter carrying no `.` has to be resolved by
iterating `UClass` for a matching name before the path is constructed.
