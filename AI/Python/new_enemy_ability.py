"""
Create a new enemy ability Blueprint and register it.
Usage: fill in the three variables at the top, then run via MCP execute_script.
"""
import unreal

ABILITY_NAME    = "GA_MyAbility"                          # e.g. "GA_DelayedFatalZone"
ABILITY_FOLDER  = "/Game/AbilitySystem/Abilities/Enemy/MyAbility"
ABILITY_TAG     = "Ability.Spell.MyAbility"               # must already exist in GeoGameplayTags.ini
PARENT_CLASS    = "/Script/GeoTrinity.GeoGameplayAbility" # change to the correct C++ base class

# --- 1. Create Blueprint ---
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.BlueprintFactory()
factory.set_editor_property("parent_class", unreal.load_class(None, PARENT_CLASS))
bp = asset_tools.create_asset(ABILITY_NAME, ABILITY_FOLDER, unreal.Blueprint, factory)
unreal.EditorAssetLibrary.save_loaded_asset(bp)

# --- 2. Set AbilityTags on CDO ---
cdo = unreal.get_default_object(bp.generated_class())
tag_container = unreal.GameplayTagContainer()
tag_container.import_text(f'(GameplayTags=((TagName="{ABILITY_TAG}")))')
cdo.set_editor_property("AbilityTags", tag_container)
unreal.EditorAssetLibrary.save_loaded_asset(bp)

# --- 3. Register in DA_AbilityInfo (triggers PopulateAbilityTags via PostEditChangeProperty) ---
da = unreal.load_asset("/Game/AbilitySystem/Data/DA_AbilityInfo")
cls_path = bp.generated_class().get_path_name()
new_info = unreal.GameplayAbilityInfo()
new_info.import_text(f'(AbilityClass="{cls_path}")')
enemy_infos = da.get_editor_property("EnemyAbilityInfos")
enemy_infos.append(new_info)
da.set_editor_property("EnemyAbilityInfos", enemy_infos)
unreal.EditorAssetLibrary.save_loaded_asset(da)

# --- 4. Add to BP_StarBoss ASC StartupAbilityTags (index 7 — verify if BP structure changes) ---
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

unreal.log(f"Done — {ABILITY_NAME} created and registered.")
