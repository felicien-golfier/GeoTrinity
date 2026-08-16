"""
Rigs SM_HexBossFrame into SKM_HexBoss on SK_HexBoss, from the rig AI/Python/generate_hex_boss_mesh.py emits.

The frame is three concentric hexagons that touch nowhere, each fringed with six spikes, and all 21 of those
pieces get a bone to themselves: turning a bone about Z spins its piece, scaling it grows it, and since no vertex
is split across bones each piece moves alone while the rest stay put. Scaling a spike bone to zero folds that
spike into the solid hexagon behind it, so it simply is not there any more.

The hierarchy is handed to the editor shim, which builds the mesh on it — bones cannot be added afterwards from
script, since the only Python route to one edits a mesh already built on a skeleton. So this script only places
the weights, which USkinWeightModifier does reach.

Where the bones go is the generator's business, not this script's — it emits their placement along with the
position of every vertex each one drives. Nothing lighter than those positions would do: a static mesh and the
skeletal mesh built from it index their vertices differently, so the vertices are re-read off the skinned mesh
and matched back to the generated point they came from.

Re-runnable: a re-run rebuilds both assets from the spec, which discards any animation authored on the old rig.

Run AFTER AI/Python/generate_hex_boss_mesh.py, via mcp-unreal execute_script.
Report written to Saved/hex_boss_rig.json.
"""
import json
import math

import unreal

APE = unreal.AnimPoseExtensions
MATH = unreal.MathLibrary

STATIC_MESH_PATH = "/Game/Art/Meshes/Boss/SM_HexBossFrame"
FOLDER = "/Game/Characters/Meshes/HexBoss"
SKELETON_NAME = "SK_HexBoss"
MESH_NAME = "SKM_HexBoss"
SPEC_FILE = "hex_boss_parts.json"
SNAP_TOLERANCE = 1.0  # a vertex further than this from every generated point is not geometry the rig describes


def get_or_create(asset_name, asset_class):
    """Load the asset if it exists, else create an empty one.

    Never deletes: deleting a loaded asset leaves the package unloadable for the rest of the editor session.
    """
    path = "{}/{}".format(FOLDER, asset_name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, asset_class, None)


def bone_arrays(bones):
    """The spec's bone table as the three parallel arrays the shim takes."""
    return ([unreal.Name(name) for name, _, _, _ in bones],
            [unreal.Name(parent) for _, parent, _, _ in bones],
            [unreal.Transform(unreal.Vector(*location), unreal.Rotator(yaw=yaw), unreal.Vector(1.0, 1.0, 1.0))
             for _, _, location, yaw in bones])


def bind_vertices(mesh, points):
    """Bind every vertex wholly to the bone of the generated point it sits on -> {bone: vertex count}.

    Positions come from the editor shim and weights from the modifier; both walk the mesh description cloned
    from LOD 0, which is what makes them line up index for index. Nearest generated point wins, and a vertex
    further than SNAP_TOLERANCE from every one of them means the mesh is not the one the spec describes.
    """
    positions = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    if len(positions) != modifier.get_num_vertices():
        raise RuntimeError("{} vertex positions against {} weighted vertices — the two do not index alike".format(
            len(positions), modifier.get_num_vertices()))

    report = {}
    for index, position in enumerate(positions):
        distance, bone = min((math.dist((x, y, z), (position.x, position.y, position.z)), bone)
                             for x, y, z, bone in points)
        if distance > SNAP_TOLERANCE:
            raise RuntimeError("vertex {} at {} sits {:.1f} away from every generated point".format(
                index, position, distance))
        modifier.set_vertex_weights(index, {unreal.Name(bone): 1.0}, True)
        report[bone] = report.get(bone, 0) + 1

    modifier.commit_weights_to_skeletal_mesh()
    return report


def rig_report(skeleton):
    """The rest pose as the rig ended up, read back off the skeleton.

    Per bone: where it sits in the boss's space, where it sits in its parent's, and the direction its local +X
    ends up pointing. The two spaces together are what shows the parenting took — a spike reads zero height in
    its hexagon's space and its hexagon's height in the boss's — and +X is the way a spike's blade slides.
    """
    pose = APE.get_reference_pose(skeleton)
    report = {}
    for bone in APE.get_bone_names(pose):
        component = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.WORLD)
        axis = MATH.transform_direction(component, unreal.Vector(1.0, 0.0, 0.0))
        local = APE.get_bone_pose(pose, str(bone), unreal.AnimPoseSpaces.LOCAL)
        report[str(bone)] = {"component": [round(v, 1) for v in (component.translation.x, component.translation.y,
                                                                 component.translation.z)],
                             "local": [round(v, 1) for v in (local.translation.x, local.translation.y,
                                                             local.translation.z)],
                             "axis": [round(v, 2) for v in (axis.x, axis.y, axis.z)]}
    return report


def main():
    result = {}
    try:
        with open(unreal.Paths.project_saved_dir() + SPEC_FILE) as f:
            spec = json.load(f)

        static_mesh = unreal.load_asset(STATIC_MESH_PATH)
        skeleton = get_or_create(SKELETON_NAME, unreal.Skeleton)
        mesh = get_or_create(MESH_NAME, unreal.SkeletalMesh)

        names, parents, transforms = bone_arrays(spec["bones"])
        if not unreal.get_default_object(unreal.GeoAnimBuilderUtil).rebuild_skeletal_mesh_from_static_mesh(
                mesh, static_mesh, skeleton, names, parents, transforms):
            raise RuntimeError("RebuildSkeletalMeshFromStaticMesh failed — see the editor log")

        result["binding"] = bind_vertices(mesh, spec["points"])
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        unreal.EditorAssetLibrary.save_loaded_asset(skeleton)

        result["rig"] = rig_report(skeleton)
        result["skeleton"] = "{}/{}".format(FOLDER, SKELETON_NAME)
        result["mesh"] = "{}/{}".format(FOLDER, MESH_NAME)
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_saved_dir() + "hex_boss_rig.json", "w") as f:
        json.dump(result, f, indent=2)


main()
