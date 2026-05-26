"""
Full setup for the DevastatingWave boss ability.
Creates GA_DevastatingWave, BP_DevastatingWavePattern, registers in DA_AbilityInfo
and adds the ability tag to BP_StarBoss ASC StartupAbilityTags.

Run AFTER editor restart (tag Ability.Spell.DevastatingWave must resolve).
"""
import unreal

ABILITY_NAME    = "GA_DevastatingWave"
PATTERN_NAME    = "BP_DevastatingWavePattern"
FOLDER          = "/Game/AbilitySystem/Abilities/Enemy/DevastatingWave"
ABILITY_TAG     = "Ability.Spell.DevastatingWave"
ABILITY_CLASS   = "/Script/GeoTrinity.GeoDevastatingWaveAbility"
PATTERN_CLASS   = "/Script/GeoTrinity.DevastatingWavePattern"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# --- 1. Create GA Blueprint ---
factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.load_class(None, ABILITY_CLASS))
bp_ability = asset_tools.create_asset(ABILITY_NAME, FOLDER, unreal.Blueprint, factory)
unreal.EditorAssetLibrary.save_loaded_asset(bp_ability)

# --- 2. Set AbilityTags on GA CDO ---
cdo = unreal.get_default_object(bp_ability.generated_class())
tag_container = unreal.GameplayTagContainer()
tag_container.import_text(f'(GameplayTags=((TagName="{ABILITY_TAG}")))')
cdo.set_editor_property("AbilityTags", tag_container)
unreal.EditorAssetLibrary.save_loaded_asset(bp_ability)

# --- 3. Create Pattern Blueprint ---
factory2 = unreal.BlueprintFactory()
factory2.set_editor_property("parent_class", unreal.load_class(None, PATTERN_CLASS))
bp_pattern = asset_tools.create_asset(PATTERN_NAME, FOLDER, unreal.Blueprint, factory2)
unreal.EditorAssetLibrary.save_loaded_asset(bp_pattern)

# --- 4. Set PatternClass on GA CDO ---
cdo.set_editor_property("PatternClass", bp_pattern.generated_class())
unreal.EditorAssetLibrary.save_loaded_asset(bp_ability)

# --- 5. Register in DA_AbilityInfo ---
da = unreal.load_asset("/Game/AbilitySystem/Data/DA_AbilityInfo")
cls_path = bp_ability.generated_class().get_path_name()
new_info = unreal.GameplayAbilityInfo()
new_info.import_text(f'(AbilityClass="{cls_path}")')
enemy_infos = da.get_editor_property("EnemyAbilityInfos")
enemy_infos.append(new_info)
da.set_editor_property("EnemyAbilityInfos", enemy_infos)
unreal.EditorAssetLibrary.save_loaded_asset(da)

# --- 6. Add tag to BP_StarBoss ASC StartupAbilityTags (index 7) ---
bp_boss = unreal.load_asset("/Game/Characters/Enemies/BP_StarBoss")
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp_boss)
d = subsystem.k2_find_subobject_data_from_handle(handles[7])
asc = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(d)
new_tag = unreal.GameplayTag()
new_tag.import_text(f'(TagName="{ABILITY_TAG}")')
tags = asc.get_editor_property("StartupAbilityTags")
tags.append(new_tag)
asc.set_editor_property("StartupAbilityTags", tags)
unreal.EditorAssetLibrary.save_loaded_asset(bp_boss)

unreal.log(f"Done — {ABILITY_NAME} and {PATTERN_NAME} created and registered.")
