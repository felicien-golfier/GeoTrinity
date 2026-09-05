"""
Read and set the AbilityTags asset-tag container on an existing ability Blueprint CDO.
Use to recover tags after reparenting a GA_ to a different C++ base, or to inspect them.
Run via MCP execute_script. Inspect results with get_output_log (pattern "ABILITYTAGS::").
"""
import unreal


def read_ability_tags(ability_path):
    """Logs the ability CDO's AbilityTags container as exported text."""
    bp = unreal.load_asset(ability_path)
    cdo = unreal.get_default_object(bp.generated_class())
    tags = cdo.get_editor_property("AbilityTags")
    unreal.log_warning(f"ABILITYTAGS:: {ability_path} -> [{tags.export_text()}]")
    return tags


def set_ability_tags(ability_path, tag_names):
    """Replaces the ability CDO's AbilityTags with tag_names (a list of full tag strings) and saves."""
    bp = unreal.load_asset(ability_path)
    cdo = unreal.get_default_object(bp.generated_class())
    container = unreal.GameplayTagContainer()
    entries = ",".join(f'(TagName="{name}")' for name in tag_names)
    container.import_text(f"(GameplayTags=({entries}))")
    cdo.set_editor_property("AbilityTags", container)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(bp)
    verify = cdo.get_editor_property("AbilityTags").export_text()
    unreal.log_warning(f"ABILITYTAGS:: {ability_path} saved={saved} -> [{verify}]")


# --- Example call (adjust path and tags) ---
set_ability_tags(
    "/Game/AbilitySystem/Abilities/Square/DeployMine/GA_Square_Special_Mine",
    ["Ability.Spell.Mine", "Ability.Type.Special"],
)
