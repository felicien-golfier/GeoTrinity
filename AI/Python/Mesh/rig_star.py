"""
Rigs SM_StarBody into SKM_Star on SK_Star, keeping the rig every star clip already drives.

The hierarchy is read off SK_Star and handed back to the editor shim unchanged — same names, same parents, same
reference pose — so every clip keyed on it plays on the new body as it did on the old one. What those clips
look like comes from how much of each vertex each bone carries, so the weights reproduce the old body's shares:

    Root ─┬─ apexes_outside  the long points' ring: half of every tip, scaling pulls the points in and out
          ├─ apexes_inside   the waist: every vertex but the tips — valleys, counter-points, the hollow heart
          ├─ mid / up        the middle / top copy's anchor, carrying whatever the two rings do not
          └─ bot ── apexe_outside_N   one per long point: the other half of that tip on the bottom copy only

A long point's tip on the bottom copy is the only vertex a tip bone moves, and the top and middle copies hide it
at rest — so a tip bone pushes a needle out of its point and never deforms the body. Everything else is one body
on the waist, the hole included, so no scale or turn of the waist can pull the outline across the hole, and a
clip turning the two rings against each other shears only the long points: a valley split between the rings
would land between two turned positions and ragged the rim.

Where each vertex sits and what it is comes from AI/Python/Mesh/generate_star_mesh.py, emitted as the spec. A
static mesh and the skeletal mesh built from it index their vertices differently, so vertices are re-read off the
skinned mesh and matched back to the generated point they came from.

Re-runnable: a re-run rebuilds the mesh and skeleton in place from the same hierarchy, so the clips survive it.

Run AFTER AI/Python/Mesh/generate_star_mesh.py, via mcp-unreal execute_script.
Report written to Saved/star_rig.json.
"""
import json
import math

import unreal

APE = unreal.AnimPoseExtensions

STATIC_MESH_PATH = "/Game/Art/Meshes/Boss/SM_StarBody"
SKELETON_PATH = "/Game/Characters/Meshes/Star/SK_Star"
MESH_PATH = "/Game/Characters/Meshes/Star/SKM_Star"
SPEC_FILE = "star_parts.json"
SNAP_TOLERANCE = 1.0  # a vertex further than this from every generated point is not geometry the spec describes

RING_BONE = "apexes_outside"
WAIST_BONE = "apexes_inside"
TIP_PREFIX = "apexe_outside_"
ANCHORS = {"bottom": "bot", "middle": "mid", "top": "up"}

RING_SHARE = 0.5                                            # of a tip, on every copy
TIP_SHARE = 0.5                                             # of a tip, on the bottom copy
WAIST_SHARE = {"bottom": 1.0, "middle": 0.75, "top": 0.75}  # of every other vertex


def hierarchy(skeleton):
    """The existing rig as (names, parents, local transforms), parents before children, the root first."""
    modifier = unreal.SkeletonModifier()
    modifier.set_skeletal_mesh(unreal.load_asset(MESH_PATH))
    reference = APE.get_reference_pose(skeleton)
    names, parents, transforms = [], [], []
    for bone in APE.get_bone_names(reference):
        local = APE.get_bone_pose(reference, str(bone), unreal.AnimPoseSpaces.LOCAL)
        names.append(unreal.Name(str(bone)))
        parents.append(modifier.get_parent_name(bone))
        transforms.append(unreal.Transform(local.translation, local.rotation.rotator(), local.scale3d))
    return names, parents, transforms


def tip_bones(skeleton):
    """Tip bone -> its rest (x, y), where its long point has to sit."""
    reference = APE.get_reference_pose(skeleton)
    tips = {}
    for bone in APE.get_bone_names(reference):
        if str(bone).startswith(TIP_PREFIX):
            location = APE.get_bone_pose(reference, str(bone), unreal.AnimPoseSpaces.WORLD).translation
            tips[str(bone)] = (location.x, location.y)
    return tips


def weights(layer, is_tip, x, y, tips):
    """One generated point's weights -> {bone: weight}, summing to 1."""
    shares = {RING_BONE: RING_SHARE} if is_tip else {WAIST_BONE: WAIST_SHARE[layer]}
    if layer == "bottom" and is_tip:
        shares[min(tips, key=lambda bone: math.dist(tips[bone], (x, y)))] = TIP_SHARE
    shares[ANCHORS[layer]] = 1.0 - sum(shares.values())
    return {bone: weight for bone, weight in shares.items() if weight > 0.001}


def check_tips(points, tips):
    """Every tip bone has to sit on the ray of a long point, or the needles it pushes leave the body sideways."""
    arms = [math.atan2(y, x) for x, y, _, layer, is_tip in points if layer == "bottom" and is_tip]
    for bone, (x, y) in tips.items():
        miss = min(abs(math.remainder(math.atan2(y, x) - angle, 2.0 * math.pi)) for angle in arms)
        if math.degrees(miss) > 0.5:
            raise RuntimeError("{} sits {:.1f} degrees off every long point".format(bone, math.degrees(miss)))


def bind_vertices(mesh, points, tips):
    """Weight every vertex from the generated point it sits on -> {bone: vertex count}.

    Positions come from the editor shim and weights from the modifier; both walk the mesh description cloned from
    LOD 0, which is what makes them line up index for index.
    """
    positions = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    if len(positions) != modifier.get_num_vertices():
        raise RuntimeError("{} vertex positions against {} weighted vertices — the two do not index alike".format(
            len(positions), modifier.get_num_vertices()))

    report = {}
    for index, position in enumerate(positions):
        distance, point = min(((math.dist((x, y, z), (position.x, position.y, position.z)), (x, y, layer, is_tip))
                               for x, y, z, layer, is_tip in points), key=lambda entry: entry[0])
        if distance > SNAP_TOLERANCE:
            raise RuntimeError("vertex {} at {} sits {:.1f} away from every generated point".format(
                index, position, distance))
        x, y, layer, is_tip = point
        vertex_weights = weights(layer, is_tip, x, y, tips)
        modifier.set_vertex_weights(index, {unreal.Name(bone): weight for bone, weight in vertex_weights.items()},
                                    True)
        for bone in vertex_weights:
            report[bone] = report.get(bone, 0) + 1

    modifier.commit_weights_to_skeletal_mesh()
    return report


def main():
    result = {}
    try:
        with open(unreal.Paths.project_saved_dir() + SPEC_FILE) as f:
            spec = json.load(f)

        skeleton = unreal.load_asset(SKELETON_PATH)
        mesh = unreal.load_asset(MESH_PATH)
        tips = tip_bones(skeleton)
        check_tips(spec["points"], tips)

        names, parents, transforms = hierarchy(skeleton)
        if not unreal.get_default_object(unreal.GeoAnimBuilderUtil).rebuild_skeletal_mesh_from_static_mesh(
                mesh, unreal.load_asset(STATIC_MESH_PATH), skeleton, names, parents, transforms):
            raise RuntimeError("RebuildSkeletalMeshFromStaticMesh failed — see the editor log")

        result["binding"] = bind_vertices(mesh, spec["points"], tips)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        unreal.EditorAssetLibrary.save_loaded_asset(skeleton)

        result["bones"] = {str(name): str(parent) for name, parent in zip(names, parents)}
        result["materials"] = [str(material.get_editor_property("material_interface").get_path_name())
                               for material in mesh.get_editor_property("materials")]
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_saved_dir() + "star_rig.json", "w") as f:
        json.dump(result, f, indent=2)


main()
