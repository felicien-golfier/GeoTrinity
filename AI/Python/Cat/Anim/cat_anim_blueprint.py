"""BS_Cat_Walk and ABP_Cat: Cat_Idle blended into BS_Cat_Walk by the float `Speed` (cm/s), played through DefaultSlot.

BS_Cat_Walk is a 1D blend space on `Speed`: Cat_Walk_Prowl at PROWL_SPEED, Cat_Walk_Fast at FAST_SPEED (cat_walks.py).
The idle gives way to it as `Speed` goes from 0 to PROWL_SPEED. DefaultSlot is where the montages (portal jump,
reassemble, idle variants) play over the locomotion. The owner writes `Speed`; nothing outside this AnimBlueprint chooses
a clip.

Run AFTER AI/Python/Cat/Mesh/rig_cat.py, cat_idle.py and cat_walks.py, via mcp-unreal execute_script. Needs the editor
shim's BuildBlendSpace1D and BuildBlendedAnimGraph. Re-runnable: the blend space and the graph are rebuilt in place.
Report written to AI/Output/cat_anim_bp.json.
"""
import json

import unreal

SKELETON = "/Game/Art/Meshes/Cat/SK_Cat"
MESH = "/Game/Art/Meshes/Cat/SKM_Cat"
ANIM_FOLDER = "/Game/Characters/Anim/Cat"
NAME = "ABP_Cat"
WALK_BLEND_SPACE = "BS_Cat_Walk"
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


def walk_speeds():
    """PROWL_SPEED, FAST_SPEED of cat_walks.py (cm/s)."""
    path = unreal.Paths.project_dir() + "AI/Python/Cat/Anim/cat_walks.py"
    walks = {"__name__": "cat_walks_library"}
    exec(compile(open(path, encoding="utf-8").read(), path, "exec"), walks)
    return walks["PROWL_SPEED"], walks["FAST_SPEED"]


def walk_blend_space(prowl_speed, fast_speed):
    """Load BS_Cat_Walk, or create it on the Cat's skeleton, and set its axis and samples."""
    blend_space = unreal.load_asset("{}/{}".format(ANIM_FOLDER, WALK_BLEND_SPACE))
    if blend_space is None:
        factory = unreal.BlendSpaceFactory1D()
        factory.set_editor_property("target_skeleton", unreal.load_asset(SKELETON))
        blend_space = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            WALK_BLEND_SPACE, ANIM_FOLDER, unreal.BlendSpace1D, factory)

    sequences = [unreal.load_asset(ANIM_FOLDER + "/Cat_Walk_Prowl"), unreal.load_asset(ANIM_FOLDER + "/Cat_Walk_Fast")]
    if not unreal.get_default_object(unreal.GeoAnimBuilderUtil).build_blend_space1d(
            blend_space, SPEED_VARIABLE, fast_speed, sequences, [prowl_speed, fast_speed]):
        raise RuntimeError("BuildBlendSpace1D failed — see the editor log")
    return blend_space


def main():
    result = {}
    try:
        blueprint = anim_blueprint()
        if blueprint is None:
            raise RuntimeError("could not create {}".format(NAME))
        prowl_speed, fast_speed = walk_speeds()
        blend_space = walk_blend_space(prowl_speed, fast_speed)
        if not unreal.get_default_object(unreal.GeoAnimBuilderUtil).build_blended_anim_graph(
                blueprint, unreal.load_asset(ANIM_FOLDER + "/Cat_Idle"), blend_space, SPEED_VARIABLE, SLOT,
                prowl_speed):
            raise RuntimeError("BuildBlendedAnimGraph failed — see the editor log")
        result["blend_space"] = blend_space.get_path_name()
        result["prowl_speed"], result["fast_speed"] = prowl_speed, fast_speed
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
