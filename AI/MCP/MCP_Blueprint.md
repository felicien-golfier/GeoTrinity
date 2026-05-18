# MCP Blueprint Creation — Unreal Python Reference

Practical knowledge from creating and configuring Blueprint assets via `mcp-unreal` `execute_script`.

---

## Prerequisites

- Unreal Editor must be open with the project **before** starting Claude Code — MCP tools are registered at session start.
- The `mcp-unreal` server is configured in `.mcp.json` at project root and allowed in `.claude/settings.local.json`.

---

## Creating Blueprint Assets

```python
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/GeoTrinity.MyClassName"))

bp = asset_tools.create_asset("BP_MyAsset", "/Game/Some/Folder", unreal.Blueprint, factory)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
```

- `load_class(None, "/Script/GeoTrinity.ClassName")` — resolves a C++ class by module + name.
- Always save with `EditorAssetLibrary.save_loaded_asset(bp)`.

---

## Setting Properties on a Blueprint CDO

```python
bp = unreal.load_asset("/Game/Path/To/BP_Asset")
cdo = unreal.get_default_object(bp.generated_class())

cdo.set_editor_property("PropertyName", value)
result = cdo.get_editor_property("PropertyName")
unreal.EditorAssetLibrary.save_loaded_asset(bp)
```

- `get_default_object(bp.generated_class())` gives the CDO — this is where Blueprint defaults live.
- `EditDefaultsOnly` private C++ properties require `meta = (AllowPrivateAccess = "true")` in the `UPROPERTY` macro to be accessible from Python. Without it, `get/set_editor_property` will throw `Failed to find property`.
- `EditDefaultsOnly` alone is enough for the property to appear in the Blueprint editor — `AllowPrivateAccess` is only needed for Python/MCP access.

---

## Setting a TSubclassOf Property

```python
target_bp = unreal.load_asset("/Game/Path/To/BP_Target")
cdo.set_editor_property("MyClassProperty", target_bp.generated_class())
```

---

## Setting a FGameplayTag Property

`FGameplayTag` is a read-only struct in Python — `TagName` cannot be assigned directly. Use `import_text`:

```python
tag = unreal.GameplayTag()
tag.import_text("(TagName=\"GameplayCue.Generic.Explosion\")")
cdo.set_editor_property("MyGameplayTagProperty", tag)

# Verify:
print(cdo.get_editor_property("MyGameplayTagProperty").export_text())
```

**Important**: A tag set this way only resolves if it is already registered in the tag manager at editor startup. Tags added to `Config/Tags/GeoGameplayTags.ini` during the session require an **editor restart** to become valid. Use an existing tag as a placeholder and swap it manually after restart.

---

## Setting a TArray<TInstancedStruct<FEffectData>> Property

Copy an `InstancedStruct` from an existing `EffectDataAsset`:

```python
da = unreal.load_asset("/Game/Path/To/DA_SomeEffectDataAsset")
instances = da.get_editor_property("EffectDataInstances")  # list of InstancedStruct
cdo.set_editor_property("ZoneEffectDataArray", [instances[0]])
```

The struct's full data (type, values, curve references) is preserved in the copy.

---

## Moving Assets

```python
unreal.EditorAssetLibrary.make_directory("/Game/New/Folder")
unreal.EditorAssetLibrary.rename_asset("/Game/Old/Path/AssetName", "/Game/New/Folder/AssetName")
```

- Redirectors are created automatically.
- Both assets are saved automatically on move.

---

## Adding a New Gameplay Tag

Edit `Config/Tags/GeoGameplayTags.ini`:

```ini
GameplayTagList=(Tag="GameplayCue.Boss.FatalZone.Countdown",DevComment="...")
```

The tag is available in-editor after the next editor restart.

---

## Accessing and Modifying Existing Blueprint Components

```python
bp = unreal.load_asset("/Game/Path/To/BP_Asset")

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
bp_handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)

# Inspect all components to find the right index
for i, h in enumerate(bp_handles):
    d = subsystem.k2_find_subobject_data_from_handle(h)
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(d)
    print(f"[{i}] {obj}")

# Get a component at a known index and modify it
comp_data = subsystem.k2_find_subobject_data_from_handle(bp_handles[2])
comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(comp_data)
comp.set_editor_property("some_property", value)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
```

- `k2_gather_subobject_data_for_blueprint(bp)` — returns list of handles; [0] = actor root, [1] = scene root, [2+] = added components
- `k2_find_subobject_data_from_handle(handle)` — resolves a handle to its data
- `SubobjectDataBlueprintFunctionLibrary.get_object(data)` — returns the raw component object (`StaticMeshComponent`, `CapsuleComponent`, etc.) — call `set_editor_property` on it directly
- Always print handles first to confirm indices before modifying

---

## Searching Assets

```python
registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path("/Game", recursive=True)
for a in assets:
    print(f"{a.asset_class_path.asset_name} | {a.package_path}/{a.asset_name}")
```

Filter by class or name substring as needed.

---

## Naming Conventions (this project)

| Asset type | Prefix | Example |
|---|---|---|
| Gameplay Ability Blueprint | `GA_` | `GA_DelayedFatalZone` |
| Pattern Blueprint | `BP_` | `BP_FatalZonePattern` |
| Effect Data Asset | `DA_` | `DA_DamageAllPlayers` |

Enemy abilities and patterns live under `/Game/AbilitySystem/Abilities/Enemy/`, one subfolder per ability.
