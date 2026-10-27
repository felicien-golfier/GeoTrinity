"""Rigs SM_Cat into SKM_Cat on SK_Cat: one rigid bone per part, the tail a chain, the PortalPoint socket on Root.

    Root
    └── Body            the pyramid; every other bone hangs under it
        ├── Head        HeadRound, HeadFur
        │   ├── EarLeft, EarRight (ear + inner ear), EyeLeft, EyeRight, Nose, Mouth0, Mouth1, FangLeft, FangRight
        ├── HandLeft, HandRight, FootLeft, FootRight
        └── Tail0 ── Tail1 ── ... ── Tail8

Every bone rests on the centre of what it carries, unturned and unscaled, so a turn or a scale on it is that key alone
(AI/MCP/MCP_Animation.md, "Editing the rig"). Root sits on the actor origin, which is the plane the portal jump sinks
through; no clip moves it, so PortalPoint stays where the portal opens.

Part bounds come from AI/Output/cat_gen.json, written by Cat/Mesh/generate_cat_mesh.py: a vertex belongs to the smallest
part box holding it. Helpers (asset creation, bones, sockets) are loaded from Badge/Mesh/rig_class_badges.py.

Run AFTER AI/Python/Cat/Mesh/generate_cat_mesh.py, via mcp-unreal execute_script. Re-runnable: rebuilds both assets, which
discards any animation authored on the old rig. Report written to AI/Output/cat_rig.json.
"""
import json

import unreal

APE = unreal.AnimPoseExtensions

FOLDER = "/Game/Art/Meshes/Cat"
ROOT = "Root"
PORTAL_SOCKET = "PortalPoint"
PORTAL_X = 110.0  # how far ahead of the origin the portal opens; the jump clip lands the Body on it
BOX_TOLERANCE = 0.05  # a vertex this far outside a part's box still belongs to it
LIBRARY = "AI/Python/Badge/Mesh/rig_class_badges.py"
GENERATED = "AI/Output/cat_gen.json"

# (bone, parent, the generated parts it carries), parents before children
TAIL = ["Tail{}".format(index) for index in range(9)]
BONES = ([(ROOT, "None", ()), ("Body", ROOT, ("Body",)), ("Head", "Body", ("HeadRound", "HeadFur"))]
         + [(ear, "Head", (ear, ear + "Inner")) for ear in ("EarLeft", "EarRight")]
         + [(part, "Head", (part,)) for part in ("EyeLeft", "EyeRight", "Nose", "Mouth0", "Mouth1", "FangLeft", "FangRight")]
         + [(part, "Body", (part,)) for part in ("HandLeft", "HandRight", "FootLeft", "FootRight")]
         + [(bone, "Body" if index == 0 else TAIL[index - 1], (bone,)) for index, bone in enumerate(TAIL)])


def load_library():
    """The badge rig's helpers as a namespace, without running its build."""
    path = unreal.Paths.project_dir() + LIBRARY
    namespace = {"__name__": "rig_library"}
    exec(compile(open(path).read(), path, "exec"), namespace)
    namespace["FOLDER"] = FOLDER
    return namespace


def box_centre(boxes):
    """The middle of the box holding every box of `boxes`."""
    return tuple((min(box["min"][axis] for box in boxes) + max(box["max"][axis] for box in boxes)) * 0.5
                 for axis in range(3))


def bone_table(bounds):
    """The rig as (bone, parent, location in parent space), each bone on the centre of its parts; also {bone: centre}."""
    centres = {ROOT: (0.0, 0.0, 0.0)}
    table = [(ROOT, "None", (0.0, 0.0, 0.0))]
    for bone, parent, parts in BONES[1:]:
        centres[bone] = box_centre([bounds[part] for part in parts])
        table.append((bone, parent, tuple(centres[bone][axis] - centres[parent][axis] for axis in range(3))))
    return table, centres


def owner_of(position, boxes):
    """The bone whose smallest part box holds `position`; each box is (volume, bone, min, max)."""
    holders = [(volume, bone) for volume, bone, low, high in boxes
               if all(low[axis] - BOX_TOLERANCE <= position[axis] <= high[axis] + BOX_TOLERANCE for axis in range(3))]
    if not holders:
        raise RuntimeError("vertex at {} sits in no part box".format(position))
    return min(holders)[1]


def bind_parts(mesh, bounds):
    """Paint every vertex wholly to the bone of the smallest part box holding it -> {bone: vertex count}."""
    boxes = []
    for bone, _, parts in BONES:
        for part in parts:
            low, high = bounds[part]["min"], bounds[part]["max"]
            volume = 1.0
            for axis in range(3):
                volume *= high[axis] - low[axis]
            boxes.append((volume, bone, low, high))

    positions = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    if len(positions) != modifier.get_num_vertices():
        raise RuntimeError("{} vertex positions against {} weighted vertices — the two do not index alike".format(
            len(positions), modifier.get_num_vertices()))

    report = {}
    for index, position in enumerate(positions):
        bone = owner_of((position.x, position.y, position.z), boxes)
        modifier.set_vertex_weights(index, {unreal.Name(bone): 1.0}, True)
        report[bone] = report.get(bone, 0) + 1
    modifier.commit_weights_to_skeletal_mesh()
    return report


def rig_cat(library, bounds):
    static_mesh = unreal.load_asset("{}/SM_Cat".format(FOLDER))
    skeleton = library["get_or_create"]("SK_Cat", unreal.Skeleton)
    mesh = library["get_or_create"]("SKM_Cat", unreal.SkeletalMesh)

    bones, centres = bone_table(bounds)
    added = library["add_missing_bones"](mesh, bones)
    if not unreal.get_default_object(unreal.GeoAnimBuilderUtil).rebuild_skeletal_mesh_from_static_mesh(
            mesh, static_mesh, skeleton,
            [unreal.Name(name) for name, _, _ in bones],
            [unreal.Name(parent) for _, parent, _ in bones],
            [unreal.Transform(unreal.Vector(*location), unreal.Rotator(), unreal.Vector(1.0, 1.0, 1.0))
             for _, _, location in bones]):
        raise RuntimeError("RebuildSkeletalMeshFromStaticMesh failed — see the editor log")

    binding = bind_parts(mesh, bounds)
    socket = library["write_socket"](mesh, PORTAL_SOCKET, ROOT, unreal.Vector(PORTAL_X, 0.0, 0.0))
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.EditorAssetLibrary.save_loaded_asset(skeleton)
    return {"mesh": mesh.get_path_name(), "skeleton": skeleton.get_path_name(), "added": added,
            "binding": binding, "socket": socket, "centres": {bone: [round(v, 2) for v in c]
                                                              for bone, c in centres.items()},
            "slots": [str(slot.material_slot_name) for slot in mesh.get_editor_property("materials")]}


def main():
    result = {}
    try:
        with open(unreal.Paths.project_dir() + GENERATED) as handle:
            bounds = json.load(handle)["parts"]
        result = rig_cat(load_library(), bounds)
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "cat_rig.json", "w") as f:
        json.dump(result, f, indent=2)


if __name__ == "__main__":
    main()
