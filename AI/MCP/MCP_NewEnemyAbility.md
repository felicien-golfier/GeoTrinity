# MCP New Enemy Ability Checklist

Steps required every time a new enemy `GA_` Blueprint ability is created.

---

## Key Constraints

- `AbilityTag` on `FGameplayAbilityInfo` is `Transient` — it is never saved and is re-populated from the CDO's `AbilityTags` on `PostLoad` / `PostEditChangeProperty`. Python must trigger that repopulation explicitly after adding the entry (see step 4).
- `AbilityClass` on `FGameplayAbilityInfo` cannot be set via `set_editor_property` — it blocks instance mutation. Use `import_text` to construct the struct with the class already embedded.
- Tags added to `Config/Tags/GeoGameplayTags.ini` during a session require an **editor restart** to resolve. Do this before step 3.

---

## Steps

### 1 — Add the gameplay tag (if new)

Edit `Config/Tags/GeoGameplayTags.ini`:
```ini
GameplayTagList=(Tag="Ability.Spell.MyAbility",DevComment="...")
```
Then **restart the editor** so the tag is registered.

### 2 — Create the Blueprint ability

```python
import unreal
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/GeoTrinity.GeoDelayedFatalZoneAbility"))  # or whichever base class
bp = asset_tools.create_asset("GA_MyAbility", "/Game/AbilitySystem/Abilities/Enemy/MyAbility", unreal.Blueprint, factory)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
```

### 3 — Set AbilityTags on the CDO

```python
cdo = unreal.get_default_object(bp.generated_class())
tag = unreal.GameplayTagContainer()
tag.import_text('(GameplayTags=((TagName="Ability.Spell.MyAbility")))')
cdo.set_editor_property("AbilityTags", tag)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
```

### 4 — Register in DA_AbilityInfo and repopulate tags

```python
da = unreal.load_asset("/Game/AbilitySystem/Data/DA_AbilityInfo")
cls_path = bp.generated_class().get_path_name()

new_info = unreal.GameplayAbilityInfo()
new_info.import_text(f'(AbilityClass="{cls_path}")')

enemy_infos = da.get_editor_property("EnemyAbilityInfos")
enemy_infos.append(new_info)
da.set_editor_property("EnemyAbilityInfos", enemy_infos)  # triggers PostEditChangeProperty → PopulateAbilityTags

unreal.EditorAssetLibrary.save_loaded_asset(da)
```

The `set_editor_property` call on `EnemyAbilityInfos` triggers `UAbilityInfo::PostEditChangeProperty`, which calls `PopulateAbilityTags()` and fills the transient `AbilityTag` from the CDO's `AbilityTags`. No separate step needed.

### 5 — Wire into the StateTree (if boss ability)

See `AI/MCP/MCP_StateTree.md` for adding a state and transitions.

---

## Existing Utility

`Source/GeoTrinity/Public/AbilitySystem/Data/AbilityInfo.h` — `UAbilityInfo` data asset.
`/Game/AbilitySystem/Data/DA_AbilityInfo` — the single instance configured in-project.
