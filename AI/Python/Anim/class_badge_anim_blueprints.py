"""One AnimBlueprint per class badge, its graph split into the two layers the badge rig paints apart.

    Top slot     -> the whole badge          except what a bottom clip holds
    Bottom slot  -> body (Root, Bottom)      over the top slot there
    DefaultSlot  -> everything, over both     death and anything else that takes the whole badge
    Additive     -> added onto all of that    an additive clip, which moves the badge on top of whatever else plays

So a top clip, the auto-fire, moves the parts and may move the body too, which shows while no bottom clip plays; a
bottom clip takes the body over and leaves the parts to the top clip, both playing at once. An additive clip plays
over a full-body one. Each slot sits in a slot group of its own, so starting one never stops the other.

The badge idle feeds the top slot, and so every layer. That slot keeps ticking it under a montage, so the idle picks
up where it would have been.

Run AFTER AI/Python/Mesh/rig_class_badges.py and AI/Python/Anim/class_badge_idle.py, via mcp-unreal
execute_script. Needs the editor shim's BuildLayeredAnimGraph. Re-runnable: the graph is rebuilt in place. Report
written to Saved/class_badge_anim_bp.json.
"""
import json

import unreal

MESH_FOLDER = "/Game/Characters/Meshes/Class"
ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
LAYER_BONE = "Top"
BOTTOM_SLOT, TOP_SLOT, FULL_BODY_SLOT, ADDITIVE_SLOT = "Bottom", "Top", "DefaultSlot", "Additive"

# Per badge: the looping idle, as an asset path, or None for the reference pose.
IDLES = {badge: "{0}/{1}/SK_{1}Badge_Sequence_Idle".format(ANIM_FOLDER, badge)
         for badge in ("Square", "Triangle", "Circle")}


def anim_blueprint(badge):
    """Load the badge's AnimBlueprint, or create it on the badge's skeleton."""
    name = "SK_{}Badge_AnimBlueprint".format(badge)
    folder = "{}/{}".format(ANIM_FOLDER, badge)
    existing = unreal.load_asset("{}/{}".format(folder, name))
    if existing:
        return existing

    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property("target_skeleton", unreal.load_asset("{}/SK_{}Badge".format(MESH_FOLDER, badge)))
    factory.set_editor_property("preview_skeletal_mesh",
                                unreal.load_asset("{}/SKM_{}Badge".format(MESH_FOLDER, badge)))
    factory.set_editor_property("parent_class", unreal.AnimInstance)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.AnimBlueprint, factory)


def main():
    result = {}
    try:
        util = unreal.get_default_object(unreal.GeoAnimBuilderUtil)
        for badge, idle in IDLES.items():
            blueprint = anim_blueprint(badge)
            if blueprint is None:
                raise RuntimeError("could not create the {} AnimBlueprint".format(badge))
            built = util.build_layered_anim_graph(blueprint, LAYER_BONE, BOTTOM_SLOT, TOP_SLOT, FULL_BODY_SLOT,
                                                  ADDITIVE_SLOT, unreal.load_asset(idle) if idle else None)
            if not built:
                raise RuntimeError("BuildLayeredAnimGraph failed for {} — see the editor log".format(badge))

            nodes = blueprint.get_nodes_of_class(unreal.AnimGraphNode_Slot, True)
            for graph_node in nodes:
                slot = graph_node.get_editor_property("node")
                if str(slot.get_editor_property("slot_name")) == TOP_SLOT:
                    slot.set_editor_property("always_update_source_pose", True)
                    graph_node.set_editor_property("node", slot)

            unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
            unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
            result[badge] = {"anim_blueprint": blueprint.get_path_name(),
                             "slots": {str(node.get_editor_property("node").get_editor_property("slot_name")):
                                       node.get_editor_property("node").get_editor_property("always_update_source_pose")
                                       for node in nodes}}
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_saved_dir() + "class_badge_anim_bp.json", "w") as f:
        json.dump(result, f, indent=2)


main()
