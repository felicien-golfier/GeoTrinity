"""
Normalizes DA_AbilityInfo's player ability entries for the ability bar. Re-runnable and idempotent — run via MCP
execute_script whenever ability icons or the ability list change.

Per player entry:
  - bShowDeployCount = AbilityClass is a UGeoDeployAbility subclass (so Turret/HealingZone/Mine slots get a count badge).
  - AbilityIcon: kept if it is a real project icon (a /Game/ texture); otherwise (unset, or an /Engine/ placeholder
    sprite) replaced with the white fallback so slots render a clean square instead of a random editor icon.

Once real icon art is added under /Game and assigned, re-running leaves it untouched.

AbilityIcon and bShowDeployCount are EditDefaultsOnly, so set_editor_property is refused on the struct copies returned
from the array. Each entry is instead cloned and overridden via import_text (a single-field import merges, preserving
the rest), then the whole array is written back. See MCP_NewEnemyAbility.md for the import_text pattern.
"""
import unreal

ABILITY_INFO_PATH = "/Game/AbilitySystem/Data/DA_AbilityInfo"
DEPLOY_BASE_PATH = "/Script/GeoTrinity.GeoDeployAbility"
WHITE_TEXTURE_PATH = "/Engine/EditorLandscapeResources/WhiteSquareTexture"
PLAYER_ARRAYS = ["triangle_abilities", "circle_abilities", "square_abilities", "shared_abilities"]


def is_real_project_icon(icon):
    """A real icon lives under /Game (project content); engine sprites and None are treated as placeholders."""
    return bool(icon) and icon.get_path_name().startswith("/Game/")


def run():
    ai = unreal.load_asset(ABILITY_INFO_PATH)
    deploy_base = unreal.load_class(None, DEPLOY_BASE_PATH)
    white = unreal.load_asset(WHITE_TEXTURE_PATH)
    white_literal = f'"{white.get_class().get_path_name()}\'{white.get_path_name()}\'"'

    for array_name in PLAYER_ARRAYS:
        rebuilt = []
        for entry in ai.get_editor_property(array_name):
            clone = unreal.PlayersGameplayAbilityInfo()
            clone.import_text(entry.export_text())

            cls = entry.get_editor_property("ability_class")
            is_deploy = bool(cls) and unreal.MathLibrary.class_is_child_of(cls, deploy_base)
            clone.import_text(f"(bShowDeployCount={'True' if is_deploy else 'False'})")

            if not is_real_project_icon(entry.get_editor_property("ability_icon")):
                clone.import_text(f"(AbilityIcon={white_literal})")

            rebuilt.append(clone)
        ai.set_editor_property(array_name, rebuilt)

    unreal.EditorAssetLibrary.save_loaded_asset(ai)
    unreal.log("ability_info_icons.py: done — deploy flags + icon fallbacks normalized on DA_AbilityInfo.")


run()
