"""The 3D gems' materials: M_Gem and one instance per mesh (MI_Gem_Chip / Cut / Prism / Core, MI_Shard), put on slot
"Gem" of SM_Gem_<Tier> and SM_Shard_<A|B|C>, and M_GemHalo on slot "Halo" of the gems (rules:
Source/GeoTrinity/Public/Gem/CLAUDE.md, "3D gems").

M_Gem is a lit, glowing stone: the vector `Color` darkened as base colour so the lights give each flat face its own
shade, and as emissive scaled by the scalar `Glow` (above 1, so bloom lights it), shaded by a fixed key light against
the pixel normal (each face flashes as the gem turns, whatever lights the level has), brightened toward grazing angles
by a fresnel rim (`RimBoost`), plus a white specular glint. `Flash` (0..1, times `FlashBoost`) lights it all up.
AGeoGemDrop sets `Color` per gem and `Flash` on a dynamic instance; a tier instance's own Color is only a default.
MI_Shard has the shard colour, fixed.

The gem meshes carry edge distances in their UVs (generate_gem_meshes.py), drawn as lit lines of width `EdgeWidth`,
whitened Color times `EdgeGlow`, over the calmer face glow, so every edge reads.

M_GemHalo is unlit and additive on the gem's halo sphere: `Color` times `HaloGlow`, full at the sphere's centre as seen
from the camera and fading to nothing at its outline (`HaloFalloff`), so the stone sits in a soft glow of its colour.
AGeoGemDrop sets its `Color` with the stone's.

Run via mcp-unreal execute_script, after AI/Python/Gem/generate_gem_meshes.py. Re-runnable: the graph is rebuilt in
place and the instances are rewritten with the values below. Report written to AI/Output/gem_materials.json.
"""
import json

import unreal

FOLDER = "/Game/Art/Mat"
MESH_FOLDER = "/Game/Art/Meshes/Gem"
SLOT = "Gem"
HALO_SLOT = "Halo"
BASE_DARKEN = 0.15  # share of Color kept as base colour
RIM_EXPONENT = 3.0
KEY_LIGHT_DIRECTION = (-0.35, 0.45, 0.82)  # unit-ish, toward the light; the camera looks down
AMBIENT = 0.3  # brightness of a face turned away from the key light
GLINT_EXPONENT = 24.0
GLINT_STRENGTH = 6.0
EDGE_WHITENING = 0.55  # share of white mixed into Color for the edge lines

WHITE = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
SHARD_COLOR = unreal.LinearColor(1.0, 0.25, 0.6, 1.0)
GLOW = {"Chip": 2.0, "Cut": 1.4, "Prism": 1.6, "Core": 2.4, "Shard": 1.4}  # faces; the edges glow on EdgeGlow
MESHES = {"Chip": ["SM_Gem_Chip"], "Cut": ["SM_Gem_Cut"], "Prism": ["SM_Gem_Prism"], "Core": ["SM_Gem_Core"],
          "Shard": ["SM_Shard_A", "SM_Shard_B", "SM_Shard_C"]}

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
mel = toolkit["mel"]
mesh_toolkit_path = unreal.Paths.project_dir() + "AI/Python/Mesh/static_mesh_asset.py"
mesh_toolkit = {}
exec(compile(open(mesh_toolkit_path).read(), mesh_toolkit_path, "exec"), mesh_toolkit)


def build_material():
    graph = toolkit["open_material"](FOLDER, "M_Gem")
    node, op = graph.node, graph.op
    vector, scalar = unreal.MaterialExpressionVectorParameter, unreal.MaterialExpressionScalarParameter
    color = graph.parameter(vector, "Color", WHITE, "01 Gem", 0, "The stat colour")
    glow = graph.parameter(scalar, "Glow", 3.0, "01 Gem", 1, "Emissive multiplier, above 1 so bloom lights it")
    rim_boost = graph.parameter(scalar, "RimBoost", 1.5, "01 Gem", 2, "Extra brightness at grazing angles")

    base = op(unreal.MaterialExpressionMultiply, color,
              B=node(unreal.MaterialExpressionConstant, r=BASE_DARKEN))
    rim = op(unreal.MaterialExpressionMultiply, node(unreal.MaterialExpressionFresnel, exponent=RIM_EXPONENT, base_reflect_fraction=0.0),
             B=rim_boost)
    flash = graph.parameter(scalar, "Flash", 0.0, "01 Gem", 3, "0..1 glow spike, set by the drop at each touchdown")
    flash_boost = graph.parameter(scalar, "FlashBoost", 3.0, "01 Gem", 4, "Extra brightness at Flash 1")

    # Lit by a fixed key light against the pixel normal, so the gem shades whatever the level's lights are.
    key_light = node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(*KEY_LIGHT_DIRECTION, 0.0))
    diffuse = op(unreal.MaterialExpressionSaturate, op(unreal.MaterialExpressionDotProduct, node(unreal.MaterialExpressionPixelNormalWS), B=key_light))
    glint_power = node(unreal.MaterialExpressionPower, const_exponent=GLINT_EXPONENT)
    graph.connect(op(unreal.MaterialExpressionSaturate, op(unreal.MaterialExpressionDotProduct, node(unreal.MaterialExpressionReflectionVectorWS), B=key_light)),
                  glint_power, "Base")
    glint = op(unreal.MaterialExpressionMultiply, glint_power,
               B=node(unreal.MaterialExpressionConstant, r=GLINT_STRENGTH))

    shade = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionAdd, rim, B=node(unreal.MaterialExpressionConstant, r=AMBIENT)),
               B=diffuse)
    brightness = op(unreal.MaterialExpressionMultiply, glow, B=shade)
    flash_gain = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, flash, B=flash_boost),
                    B=node(unreal.MaterialExpressionConstant, r=1.0))
    face = op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionMultiply, color, B=brightness), B=glint)

    # The mesh carries each pixel's distance to the solid's edges in UV0.xy (generate_gem_meshes.py).
    edge_width = graph.parameter(scalar, "EdgeWidth", 2.0, "02 Edges", 0, "Width of the lit edges, in mesh units")
    edge_glow = graph.parameter(scalar, "EdgeGlow", 7.0, "02 Edges", 1, "Emissive multiplier of the edges")
    uv0 = node(unreal.MaterialExpressionTextureCoordinate, coordinate_index=0)
    edge_distance = op(unreal.MaterialExpressionMin, graph.mask(uv0, "r"),
                       B=graph.mask(uv0, "g"))
    edge_mask = op(unreal.MaterialExpressionSaturate, op(unreal.MaterialExpressionSubtract, edge_width, B=edge_distance))
    edge_color = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionLinearInterpolate, color,
                       B=node(unreal.MaterialExpressionConstant3Vector, constant=WHITE),
                       Alpha=node(unreal.MaterialExpressionConstant, r=EDGE_WHITENING)),
                    B=edge_glow)
    lit = op(unreal.MaterialExpressionLinearInterpolate, face, B=edge_color, Alpha=edge_mask)
    emissive = op(unreal.MaterialExpressionMultiply, lit, B=flash_gain)

    graph.to_property(base, unreal.MaterialProperty.MP_BASE_COLOR)
    graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    graph.to_property(node(unreal.MaterialExpressionConstant, r=0.3), unreal.MaterialProperty.MP_ROUGHNESS)

    graph.finish()
    return graph.asset


def build_halo_material():
    graph = toolkit["open_material"](FOLDER, "M_GemHalo")
    node, op = graph.node, graph.op
    material = graph.asset
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    color = graph.parameter(unreal.MaterialExpressionVectorParameter, "Color", WHITE, "01 Halo", 0,
                            "The stat colour")
    halo_glow = graph.parameter(unreal.MaterialExpressionScalarParameter, "HaloGlow", 1.0, "01 Halo", 1,
                                "Emissive multiplier at the centre")
    falloff = graph.parameter(unreal.MaterialExpressionScalarParameter, "HaloFalloff", 1.5, "01 Halo", 2,
                              "Fade toward the outline, higher = tighter around the stone")

    # facing = cos of the angle to the camera; the sphere point lies at sqrt(1 - facing^2) of the radius off-centre.
    facing = op(unreal.MaterialExpressionSaturate, op(unreal.MaterialExpressionDotProduct, node(unreal.MaterialExpressionVertexNormalWS),
                   B=node(unreal.MaterialExpressionCameraVectorWS)))
    off_centre = op(unreal.MaterialExpressionSquareRoot, op(unreal.MaterialExpressionOneMinus, op(unreal.MaterialExpressionMultiply, facing, B=facing)))
    fade = op(unreal.MaterialExpressionPower, op(unreal.MaterialExpressionOneMinus, off_centre), Exp=falloff)
    emissive = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, color, B=halo_glow), B=fade)
    graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    graph.finish()
    return material


def build_instance(name, parent, color, glow):
    is_new = not unreal.EditorAssetLibrary.does_asset_exist("{}/{}".format(FOLDER, name))
    instance = toolkit["load_or_create"](FOLDER, name, unreal.MaterialInstanceConstant,
                                         unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, parent)
    if is_new:
        mel.set_material_instance_vector_parameter_value(instance, "Color", color)

    mel.set_material_instance_scalar_parameter_value(instance, "Glow", glow)

    mel.update_material_instance(instance)
    toolkit["save"](instance)
    return instance


def put_on_meshes(kind, worn_by_slot):
    """Each mesh of this kind wears worn_by_slot[slot name] on every slot."""
    worn = {}
    for mesh_name in MESHES[kind]:
        mesh = mesh_toolkit["load_mesh"]("{}/{}".format(MESH_FOLDER, mesh_name))
        slots = list(mesh.get_editor_property("static_materials"))
        for slot in slots:
            slot_name = str(slot.get_editor_property("material_slot_name"))
            assert slot_name in worn_by_slot, "{}: unexpected slot {}".format(mesh_name, slot_name)
            slot.set_editor_property("material_interface", worn_by_slot[slot_name])

        mesh.set_editor_property("static_materials", slots)
        mesh_toolkit["save_mesh"](mesh)
        worn[mesh_name] = [worn_by_slot[str(slot.get_editor_property("material_slot_name"))].get_name() for slot in slots]

    return worn


def main():
    result = {}
    try:
        material = build_material()
        halo = build_halo_material()
        result["slots"] = {}
        for kind in MESHES:
            name = "MI_Shard" if kind == "Shard" else "MI_Gem_" + kind
            instance = build_instance(name, material, SHARD_COLOR if kind == "Shard" else WHITE, GLOW[kind])
            result["slots"].update(put_on_meshes(kind, {SLOT: instance, HALO_SLOT: halo}))

        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "gem_materials.json", "w") as f:
        json.dump(result, f, indent=2)


main()
