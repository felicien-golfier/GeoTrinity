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

A boolean property answers to its C++ name with the `b` kept, and to its snake_case name with the `b` dropped, but never to the `b`-less PascalCase form.

The correct CDO accessor is `unreal.get_default_object(bp.generated_class())` — calling `get_default_object()` directly on the class object fails.

Property names passed to `set_editor_property` / `get_editor_property` are **PascalCase** (matching the C++ `UPROPERTY` name), not snake_case.

Resolve a class by its script path with the class loader; there is no find-by-name helper.

Whether a class is in the running build is answered by that path lookup and never by the module's attributes, since a reflected class is not always mirrored as one.

The object factory takes a class loaded that way as readily as a module attribute, so a class the module omits can still be instanced.

An instanced subobject property only reaches spawned instances when it holds the default subobject the owning C++ class creates under that name; one assigned afterwards stays on the class defaults and every instance gets none, whatever the assigned object is named. Declare the subobject's class in C++ and let script author only its properties.

A soft-object-pointer property takes the loaded asset object as its value; passing a soft-object-path value fails type conversion.

A class absent from the Python module can still be reached by loading its CDO via the script path with the `Default__` prefix; its reflected functions are callable with the by-name method caller, which also works for static function libraries.

A byte property backed by an unexposed enum cannot be read or written from Python, and the Remote Control API refuses private properties — changing one needs a C++ shim.

A gameplay-tag container is read as exported text with its `export_text` method (there is no `to_string`), and written by building a fresh container and round-tripping the desired tags through its `import_text` method.

## Editing a Struct Container

Elements read from a struct array or a struct map are by-value copies, so mutating one in place does not write back.

`EditDefaultsOnly` struct fields reject the property setter even on the copy; clone the element by round-tripping its exported text into a fresh struct, then merge overrides — a single-field text import sets only that field and preserves the rest. Reassign the whole rebuilt container to the asset and save.

An object reference merges in as its exported text, never as the asset itself.

A container edit can leave the package clean, so save it unconditionally rather than only when dirty.

See `AI/Python/struct_container_edit.py` for both container kinds, and `AI/Python/ability_info_icons.py` for a worked array case.

## Authoring a Curve Asset

A curve asset holds its channels in a fixed-size array the reflection system does not expose, so the keys go in through the CSV importer rather than the property setter.

The importer reads one row per key — a time followed by one value per channel, with no header — and how many values a row carries is what picks the curve class.

Mark the import task automated, or the options dialog stalls an unattended run.

See `AI/Python/curve_asset_authoring.py` for the writer.

## Naming Conventions

| Asset type | Prefix |
|---|---|
| Gameplay Ability Blueprint | `GA_` |
| Pattern Blueprint | `BP_` |
| Effect Data Asset | `DA_` |
| Widget Blueprint | `WBP_` |

Enemy abilities live under `/Game/AbilitySystem/Abilities/Enemy/`, one subfolder per ability.
HUD widgets live under `/Game/HUD/`. For widget-specific automation see `MCP_UI.md`.
