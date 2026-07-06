# MCP Blueprint Creation — Unreal Python Reference

Practical knowledge for creating and configuring Blueprint assets via `mcp-unreal` `execute_script`.

## Prerequisites

- Unreal Editor must be open before starting Claude Code.
- Tags added to `Config/Tags/GeoGameplayTags.ini` require an **editor restart** to resolve.

## Key Patterns

See `AI/Python/` for call patterns covering:
- Creating a Blueprint asset via `AssetTools` and a factory
- Setting CDO properties via `get_default_object`
- Setting a `FGameplayTag` via `import_text`
- Setting a `TSubclassOf` property
- Accessing and mutating subobject components via `SubobjectDataSubsystem` — save with `EditorAssetLibrary.save_loaded_asset(asset)`
- Copying a `TInstancedStruct` from an existing asset
- Moving an asset
- Deleting an asset via `EditorAssetLibrary`
- Searching assets by keyword via `AssetRegistry`

**`EditDefaultsOnly` private properties** need `meta=(AllowPrivateAccess="true")` to be accessible from Python. When the property is in project C++ code, add the meta specifier directly — do not use workarounds.

## Property Access

A boolean property's `b` prefix is dropped in its Python name.

The correct CDO accessor is `unreal.get_default_object(bp.generated_class())` — calling `get_default_object()` directly on the class object fails.

Property names passed to `set_editor_property` / `get_editor_property` are **PascalCase** (matching the C++ `UPROPERTY` name), not snake_case.

Resolve a class by its script path with the class loader; there is no find-by-name helper.

A soft-object-pointer property takes the loaded asset object as its value; passing a soft-object-path value fails type conversion.

A class absent from the Python module can still be reached by loading its CDO via the script path with the `Default__` prefix; its reflected functions are callable with the by-name method caller, which also works for static function libraries.

A byte property backed by an unexposed enum cannot be read or written from Python, and the Remote Control API refuses private properties — changing one needs a C++ shim.

A gameplay-tag container is read as exported text with its `export_text` method (there is no `to_string`), and written by building a fresh container and round-tripping the desired tags through its `import_text` method.

## Editing a Struct Array

Elements read from a struct array are by-value copies, so mutating one in place does not write back.

`EditDefaultsOnly` struct fields reject the property setter even on the copy; clone the element by round-tripping its exported text into a fresh struct, then merge overrides — a single-field text import sets only that field and preserves the rest. Reassign the whole rebuilt array to the asset and save. See `AI/Python/ability_info_icons.py`.

## Naming Conventions

| Asset type | Prefix |
|---|---|
| Gameplay Ability Blueprint | `GA_` |
| Pattern Blueprint | `BP_` |
| Effect Data Asset | `DA_` |
| Widget Blueprint | `WBP_` |

Enemy abilities live under `/Game/AbilitySystem/Abilities/Enemy/`, one subfolder per ability.
HUD widgets live under `/Game/HUD/`. For widget-specific automation see `MCP_UI.md`.
