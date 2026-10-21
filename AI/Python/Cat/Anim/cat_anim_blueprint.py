"""ABP_Cat: Cat_Idle and Cat_Walk blended by the float `Speed` (0 idle, 1 walk), played through DefaultSlot.

DefaultSlot is where AM_Cat_PortalJump plays over the locomotion. The owner writes `Speed`; nothing outside this
AnimBlueprint chooses a clip.

Run AFTER AI/Python/Cat/Mesh/rig_cat.py, cat_idle.py and cat_walk.py, via mcp-unreal execute_script. Needs the editor shim's
BuildBlendedAnimGraph. Re-runnable: the graph is rebuilt in place. Report written to AI/Output/cat_anim_bp.json.
"""
import json

import unreal

SKELETON = "/Game/Art/Meshes/Cat/SK_Cat"
MESH = "/Game/Art/Meshes/Cat/SKM_Cat"
ANIM_FOLDER = "/Game/Characters/Anim/Cat"
NAME = "ABP_Cat"
SPEED_VARIABLE = "Speed"
SLOT = "DefaultSlot"


def anim_blueprint():
    """Load ABP_Cat, or create it on the Cat's skeleton."""
    existing = unreal.load_asset("{}/{}".format(ANIM_FOLDER, NAME))
    if existing:
        return existing

    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON))
    factory.set_editor_property("preview_skeletal_mesh", unreal.load_asset(MESH))
    factory.set_editor_property("parent_class", unreal.AnimInstance)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, ANIM_FOLDER, unreal.AnimBlueprint, factory)


def main():
    result = {}
    try:
        blueprint = anim_blueprint()
        if blueprint is None:
            raise RuntimeError("could not create {}".format(NAME))
        if not unreal.get_default_object(unreal.GeoAnimBuilderUtil).build_blended_anim_graph(
                blueprint, unreal.load_asset(ANIM_FOLDER + "/Cat_Idle"), unreal.load_asset(ANIM_FOLDER + "/Cat_Walk"),
                SPEED_VARIABLE, SLOT):
            raise RuntimeError("BuildBlendedAnimGraph failed — see the editor log")
        result["anim_blueprint"] = blueprint.get_path_name()
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "cat_anim_bp.json", "w") as f:
        json.dump(result, f, indent=2)


main()
