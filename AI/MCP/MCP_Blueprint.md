# MCP Blueprint Creation — Unreal Python Reference

Practical knowledge for creating and configuring Blueprint assets via `mcp-unreal` `execute_script`.

## Prerequisites

- Unreal Editor must be open before starting Claude Code.
- Tags added to `Config/Tags/GeoGameplayTags.ini` require an **editor restart** to resolve.

## Key Patterns

**Create a Blueprint asset**
```python
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/GeoTrinity.MyClass"))
bp = asset_tools.create_asset("BP_Name", "/Game/Some/Folder", unreal.Blueprint, factory)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
```

**Set a CDO property** — `get_default_object(bp.generated_class())` gives the CDO.
`EditDefaultsOnly` private properties need `meta=(AllowPrivateAccess="true")` to be accessible from Python.

**Set a `FGameplayTag`** — read-only struct, use `import_text`:
```python
tag = unreal.GameplayTag()
tag.import_text('(TagName="Ability.Spell.X")')
```

**Set a `TSubclassOf`**
```python
cdo.set_editor_property("MyClassProp", target_bp.generated_class())
```

**Access a component** — gather subobject handles, print indices first to confirm, then get and mutate:
```python
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
for i, h in enumerate(handles):
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(subsystem.k2_find_subobject_data_from_handle(h))
    print(f"[{i}] {obj}")
```

**Copy a `TInstancedStruct` from an existing asset**
```python
da = unreal.load_asset("/Game/Path/To/DA_Asset")
instances = da.get_editor_property("EffectDataInstances")
cdo.set_editor_property("MyArray", [instances[0]])
```

**Move an asset**
```python
unreal.EditorAssetLibrary.rename_asset("/Game/Old/Path/Asset", "/Game/New/Folder/Asset")
```

**Search assets**
```python
registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path("/Game", recursive=True)
for a in assets:
    if "keyword" in str(a.asset_name).lower():
        unreal.log(f"{a.asset_class_path.asset_name} | {a.package_path}/{a.asset_name}")
```

## Naming Conventions

| Asset type | Prefix |
|---|---|
| Gameplay Ability Blueprint | `GA_` |
| Pattern Blueprint | `BP_` |
| Effect Data Asset | `DA_` |

Enemy abilities live under `/Game/AbilitySystem/Abilities/Enemy/`, one subfolder per ability.
