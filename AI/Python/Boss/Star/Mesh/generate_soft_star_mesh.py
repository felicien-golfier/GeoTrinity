"""
Generates SKM_SoftStar, the friendly dummy's body: the star's eight points rounded off, a pale core disc on top.

Same footprint as SKM_Star (tips at 70.71, height 100) and the same rig: a copy of SKM_Star rebuilt in place on SK_Star,
every vertex on the waist bone, so BP_Dummy's animation Blueprint drives it as it drives the sharp star. Slot Color =
the body (replaced by BP_Dummy's FriendlyBodyMaterial), slot Core = the pale disc, MI_SoftStar_Core, kept as is.

Also makes MI_SoftStar_Body (ally green) and MI_SoftStar_Core (pale), instances of the enemy star's MAT_EnemyMat.
Re-runnable. Run via mcp-unreal execute_script or headless (AI/Commands.md). Report: AI/Output/soft_star.json.
"""
import json
import math

import unreal

mesh_toolkit_path = unreal.Paths.project_dir() + "AI/Python/Mesh/static_mesh_asset.py"
mesh_toolkit = {}
exec(compile(open(mesh_toolkit_path).read(), mesh_toolkit_path, "exec"), mesh_toolkit)

APE = unreal.AnimPoseExtensions
STATIC_FOLDER = "/Game/Art/Meshes/Boss"
STATIC_NAME = "SM_SoftStarBody"
SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
SOURCE_MESH = "/Game/Characters/Meshes/Star/SKM_Star"
MESH_FOLDER = "/Game/Characters/Meshes/Star"
MESH_NAME = "SKM_SoftStar"
MATERIAL_FOLDER = "/Game/Characters/Meshes/Star"
PARENT_MATERIAL = "/Game/Characters/Meshes/Star/MAT_EnemyMat"
BODY_SLOT, CORE_SLOT = "Color", "Core"
BODY_COLOR, CORE_COLOR = "#4dff6a", "#eaffe9"
WAIST_BONE = "apexes_inside"

POINTS = 8
TIP_RADIUS = 70.71
VALLEY_RADIUS = 40.0
CORE_RADIUS = 20.0
HALF_HEIGHT = 50.0
SEGMENTS = POINTS * 12


def outline_radius(angle):
    """Eight smooth lobes: a tip at angle 0, a valley half a point later."""
    return VALLEY_RADIUS + (TIP_RADIUS - VALLEY_RADIUS) * 0.5 * (1.0 + math.cos(POINTS * angle))


def linear(hex_color):
    """sRGB hex -> LinearColor."""
    channels = [int(hex_color[i:i + 2], 16) / 255.0 for i in (1, 3, 5)]
    channels = [c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in channels]
    return unreal.LinearColor(channels[0], channels[1], channels[2], 1.0)


def build_static_mesh():
    desc = unreal.StaticMesh().create_static_mesh_description()
    body = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(body, BODY_SLOT)
    core = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(core, CORE_SLOT)

    def vertex(x, y, z):
        created = desc.create_vertex()
        desc.set_vertex_position(created, unreal.Vector(x, y, z))
        return created

    def tri(group, a, b, c):
        # Wound CCW seen from outside; UE front faces are clockwise, so reverse here.
        desc.create_triangle(group, [desc.create_vertex_instance(c), desc.create_vertex_instance(b),
                                     desc.create_vertex_instance(a)])

    angles = [2.0 * math.pi * i / SEGMENTS for i in range(SEGMENTS)]
    rim = [(outline_radius(a) * math.cos(a), outline_radius(a) * math.sin(a)) for a in angles]
    inner = [(CORE_RADIUS * math.cos(a), CORE_RADIUS * math.sin(a)) for a in angles]
    top_rim = [vertex(x, y, HALF_HEIGHT) for x, y in rim]
    bottom_rim = [vertex(x, y, -HALF_HEIGHT) for x, y in rim]
    top_inner = [vertex(x, y, HALF_HEIGHT) for x, y in inner]
    bottom_center = vertex(0.0, 0.0, -HALF_HEIGHT)
    top_center = vertex(0.0, 0.0, HALF_HEIGHT)

    for i in range(SEGMENTS):
        n = (i + 1) % SEGMENTS
        tri(body, bottom_rim[i], bottom_rim[n], top_rim[n])
        tri(body, bottom_rim[i], top_rim[n], top_rim[i])
        tri(body, top_rim[i], top_rim[n], top_inner[n])
        tri(body, top_rim[i], top_inner[n], top_inner[i])
        tri(body, bottom_center, bottom_rim[n], bottom_rim[i])
        tri(core, top_center, top_inner[i], top_inner[n])

    materials = [unreal.StaticMaterial(material_interface=unreal.load_asset(f"{MATERIAL_FOLDER}/MI_SoftStar_Body"),
                                       material_slot_name=BODY_SLOT),
                 unreal.StaticMaterial(material_interface=unreal.load_asset(f"{MATERIAL_FOLDER}/MI_SoftStar_Core"),
                                       material_slot_name=CORE_SLOT)]
    return mesh_toolkit["save_static_mesh"](STATIC_FOLDER, STATIC_NAME, desc, materials)


def make_instance(name, color):
    path = f"{MATERIAL_FOLDER}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        instance = unreal.load_asset(path)
    else:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, MATERIAL_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    instance.set_editor_property("parent", unreal.load_asset(PARENT_MATERIAL))
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(instance, "Base Color", color)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)


def hierarchy(skeleton, mesh):
    """The rig of SK_Star as (names, parents, local transforms), unchanged."""
    modifier = unreal.SkeletonModifier()
    modifier.set_skeletal_mesh(mesh)
    reference = APE.get_reference_pose(skeleton)
    names, parents, transforms = [], [], []
    for bone in APE.get_bone_names(reference):
        local = APE.get_bone_pose(reference, str(bone), unreal.AnimPoseSpaces.LOCAL)
        names.append(unreal.Name(str(bone)))
        parents.append(modifier.get_parent_name(bone))
        transforms.append(unreal.Transform(local.translation, local.rotation.rotator(), local.scale3d))
    return names, parents, transforms


def rig(static_mesh):
    path = f"{MESH_FOLDER}/{MESH_NAME}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MESH, path)
    mesh = unreal.load_asset(path)
    skeleton = unreal.load_asset(SKELETON_PATH)
    names, parents, transforms = hierarchy(skeleton, mesh)
    if not unreal.get_default_object(unreal.GeoAnimBuilderUtil).rebuild_skeletal_mesh_from_static_mesh(
            mesh, static_mesh, skeleton, names, parents, transforms):
        raise RuntimeError("RebuildSkeletalMeshFromStaticMesh failed — see the editor log")

    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    for index in range(modifier.get_num_vertices()):
        modifier.set_vertex_weights(index, {unreal.Name(WAIST_BONE): 1.0}, True)

    modifier.commit_weights_to_skeletal_mesh()
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    return mesh


def main():
    result = {}
    try:
        make_instance("MI_SoftStar_Body", linear(BODY_COLOR))
        make_instance("MI_SoftStar_Core", linear(CORE_COLOR))
        static_mesh = build_static_mesh()
        mesh = rig(static_mesh)
        result["ok"] = True
        result["bounds"] = str(mesh.get_bounds().box_extent)
        result["materials"] = [str(m.get_editor_property("material_slot_name")) + "=" +
                               str(m.get_editor_property("material_interface").get_path_name())
                               for m in mesh.get_editor_property("materials")]
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_dir() + "AI/Output/soft_star.json", "w") as f:
        json.dump(result, f, indent=2)


main()
