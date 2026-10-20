"""The Tutorial Cat's materials: M_Cat and one instance per colour role (Base, Shade, Ink, Light), put on SKM_Cat.

M_Cat is flat and unlit: the role's colour as emissive, masked by world height. Every pixel below the actor origin
height is dropped, so a part sinking through the actor origin vanishes as it crosses (AI/Tutorial/CatAnimation.md,
"Portal jump"). Colour defaults are the Cat face widget's (GeoCatFaceWidget.h), so the 3D Cat and the HUD face match.

Run via mcp-unreal execute_script, after AI/Python/Mesh/rig_cat.py. Re-runnable: the graph is rebuilt in place and an
existing instance keeps the colour tuned on it. Report written to AI/Output/cat_materials.json.
"""
import json

import unreal

FOLDER = "/Game/Art/Meshes/Cat"
MESH = FOLDER + "/SKM_Cat"
CUT_SHARPNESS = 1000.0  # height above the origin (units) that reads as fully visible is 1 / this

# Linear colours of the face widget's four roles.
ROLES = {"Base": unreal.LinearColor(1.0, 0.102, 0.047, 1.0),
         "Shade": unreal.LinearColor(0.55, 0.056, 0.026, 1.0),
         "Ink": unreal.LinearColor(0.12, 0.012, 0.006, 1.0),
         "Light": unreal.LinearColor(1.0, 0.87, 0.84, 1.0)}

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
mel = toolkit["mel"]


def build_material():
    graph = toolkit["open_material"](FOLDER, "M_Cat")
    node, op = graph.node, graph.op
    color = graph.parameter(unreal.MaterialExpressionVectorParameter, "Color", ROLES["Base"], "01 Cat", 0,
                            "The colour role, emissive", -300, 0)
    height = op(unreal.MaterialExpressionSubtract, -200, 300,
                graph.mask(node(unreal.MaterialExpressionWorldPosition, -600, 300), "b", -400, 300),
                B=graph.mask(node(unreal.MaterialExpressionActorPositionWS, -600, 450), "b", -400, 450))
    visible = op(unreal.MaterialExpressionSaturate, 200, 300,
                 op(unreal.MaterialExpressionMultiply, 0, 300, height, B=node(unreal.MaterialExpressionConstant, -200, 450,
                                                                               r=CUT_SHARPNESS)))
    graph.to_property(color, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    graph.to_property(visible, unreal.MaterialProperty.MP_OPACITY_MASK)

    material = graph.asset
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_skeletal_mesh", True)
    graph.finish()
    assert mel.get_statistics(material).num_pixel_shader_instructions > 0, "M_Cat did not compile, see the editor"
    return material


def build_instance(role, parent):
    name = "MI_Cat_" + role
    is_new = not unreal.EditorAssetLibrary.does_asset_exist("{}/{}".format(FOLDER, name))
    instance = toolkit["load_or_create"](FOLDER, name, unreal.MaterialInstanceConstant,
                                         unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, parent)
    if is_new:
        mel.set_material_instance_vector_parameter_value(instance, "Color", ROLES[role])

    mel.update_material_instance(instance)
    toolkit["save"](instance)
    return instance


def put_on_mesh(instances):
    """Each material slot named after a role wears that role's instance."""
    mesh = toolkit["load"](MESH)
    slots = list(mesh.get_editor_property("materials"))
    for slot in slots:
        slot.set_editor_property("material_interface", instances[str(slot.get_editor_property("material_slot_name"))])

    mesh.set_editor_property("materials", slots)
    toolkit["save"](mesh)
    return {str(slot.get_editor_property("material_slot_name")): slot.get_editor_property("material_interface").get_name()
            for slot in mesh.get_editor_property("materials")}


def main():
    result = {}
    try:
        material = build_material()
        instances = {role: build_instance(role, material) for role in ROLES}
        result["slots"] = put_on_mesh(instances)
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "cat_materials.json", "w") as f:
        json.dump(result, f, indent=2)


main()
