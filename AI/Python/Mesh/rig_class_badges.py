"""Rigs the three class badges into SKM_<Class>Badge on SK_<Class>Badge, ready for two animation layers.

Every badge splits into a body and the parts floating free around it, and the rig splits the same way:

    Root              on the badge's origin, the character's pivot
    ├── Bottom        the body — the bottom layer
    └── Top           a pivot on the badge's centre, carrying no vertex — the top layer
        └── <part>    one bone per floating part, at that part's own centre

Bottom and Top stand on the centre of the badge's box, which is the origin unless the generator centres the badge on
its centroid (the Triangle), so a clip authored about the box centre keeps playing about it.

Each vertex is painted wholly to one bone, so a layered blend filtered on the Top branch plays a top clip on the
parts while a bottom clip keeps the body, and neither reaches the other. A part bone sits on its part's own centre so
a turn or a scale works in place; turning Top swings every part round the badge's centre instead.

The fire sockets the abilities read, anim_socket_<n>, hang off the root a fixed FIRE_SOCKET_REACH ahead, so a shot
leaves the same point whatever the clip does to the part that seems to fire it.

Outlines come from AI/Python/Mesh/generate_class_badge_meshes.py, loaded rather than restated: a vertex belongs to
the part whose outline holds it, which needs no point list since the parts never touch. Parts stacked over one
another share an outline, so between those the one standing nearest the vertex's height takes it; a lifted part's
bone stands at its own height.

Re-runnable: a re-run rebuilds both assets from the badges, which discards any animation authored on the old rig.

Run AFTER AI/Python/Mesh/generate_class_badge_meshes.py, via mcp-unreal execute_script.
Report written to Saved/class_badge_rig.json.
"""
import json
import math

import unreal

APE = unreal.AnimPoseExtensions

FOLDER = "/Game/Characters/Meshes/Class"
GENERATOR = "AI/Python/Mesh/generate_class_badge_meshes.py"
ROOT, BOTTOM, TOP = "Root", "Bottom", "Top"
LAYER_LIFT = 20.0  # Bottom and Top sit this far below and above the root, so the three stay pickable
SNAP_TOLERANCE = 1.0  # a vertex further than this from every outline is not geometry the badge describes
DEFAULT_SOCKET_NAME = "Socket"  # what a socket constructed from script is called
FIRE_SOCKET_REACH = 50.0  # how far ahead of the origin every fire socket sits

# Per badge: the name of each floating part in generator order (the body comes first and is not named), and each
# fire socket's sideways (Y) offset on the root, in socket order. anim_socket_0 is the first shot, then 1, 2 cycling
# on 1. "keyhole_socket" names a socket on the body at the centre of the Square block's keyhole, where the sacrifice
# goes in and comes back out.
BADGES = {
    "Square": {"parts": ["MandibleLeft", "MandibleRight"], "sockets": [43.0, -43.0, 43.0],
               "keyhole_socket": "SacrificeHole"},
    "Triangle": {"parts": ["Needle", "Plug"], "sockets": [0.0, 0.0]},
    "Circle": {"parts": ["Hourglass"], "sockets": [0.0]},
}


def load_generator():
    """The badge generator's namespace, without running its build."""
    path = unreal.Paths.project_dir() + GENERATOR
    namespace = {"__name__": "badge_generator"}
    exec(compile(open(path).read(), path, "exec"), namespace)
    return namespace


def world_parts(generator, static_mesh_name):
    """Every part of one badge in world space, the body first."""
    return generator["to_world"](static_mesh_name)


def centre(outline):
    """The middle of an outline's box: a disc's centre, and the waist of the needle and of a bar alike."""
    xs, ys = [p[0] for p in outline], [p[1] for p in outline]
    return (min(xs) + max(xs)) * 0.5, (min(ys) + max(ys)) * 0.5


def distance_to_outline(point, outline):
    """0 inside the outline, else the distance to its nearest edge."""
    x, y = point
    inside = False
    nearest = float("inf")
    for (ax, ay), (bx, by) in zip(outline, outline[1:] + outline[:1]):
        if (ay > y) != (by > y) and x < ax + (y - ay) * (bx - ax) / (by - ay):
            inside = not inside
        dx, dy = bx - ax, by - ay
        t = max(0.0, min(1.0, ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy)))
        nearest = min(nearest, math.hypot(x - ax - t * dx, y - ay - t * dy))
    return 0.0 if inside else nearest


def box_centre(outlines):
    """The centre of the whole badge's box, where Bottom and Top stand."""
    return centre([point for outline in outlines for point in outline])


def bone_table(part_names, parts):
    """The rig as (bone, parent, location in parent space), parents before children, the root first."""
    box_x, box_y = box_centre([part.outline for part in parts])
    bones = [(ROOT, "None", (0.0, 0.0, 0.0)),
             (BOTTOM, ROOT, (box_x, box_y, -LAYER_LIFT)),
             (TOP, ROOT, (box_x, box_y, LAYER_LIFT))]
    for name, part in zip(part_names, parts[1:]):
        x, y = centre(part.outline)
        bones.append((name, TOP, (x - box_x, y - box_y, part.lift - LAYER_LIFT)))
    return bones


def get_or_create(asset_name, asset_class):
    """Load the asset if it exists, else create an empty one.

    Never deletes: a deleted loaded package stays unloadable.
    """
    asset = unreal.load_asset("{}/{}".format(FOLDER, asset_name))
    return asset or unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, asset_class, None)


def add_missing_bones(mesh, bones):
    """Adds each bone of the table the mesh does not carry yet as a leaf, through the skeleton modifier -> [name].

    The rebuild only ever re-places bones: growing the count there rebuilds a mesh its skeleton cannot map yet, which
    brings the editor down. Leaves added in table order also keep the order the rebuild then writes.
    """
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton is None:
        return []  # a mesh created this run: nothing is live on it yet
    existing = {str(name) for name in APE.get_bone_names(APE.get_reference_pose(skeleton))}
    missing = [(name, parent, location) for name, parent, location in bones if name not in existing]
    if missing:
        modifier = unreal.SkeletonModifier()
        modifier.set_skeletal_mesh(mesh)
        modifier.add_bones([unreal.Name(name) for name, _, _ in missing],
                           [unreal.Name(parent) for _, parent, _ in missing],
                           [unreal.Transform(unreal.Vector(*location)) for _, _, location in missing])
        modifier.commit_skeleton_to_skeletal_mesh()
    return [name for name, _, _ in missing]


def bind_vertices(mesh, owners):
    """Paint every vertex wholly to the bone of the outline holding it -> {bone: vertex count}.

    `owners` is [(bone, outline, lift)]. Positions come from the editor shim and weights from the modifier; both walk
    the mesh description cloned from LOD 0, which is what makes them line up index for index.
    """
    positions = unreal.get_default_object(unreal.GeoAnimBuilderUtil).get_skeletal_mesh_vertex_positions(mesh)
    modifier = unreal.SkinWeightModifier()
    modifier.set_skeletal_mesh(mesh)
    if len(positions) != modifier.get_num_vertices():
        raise RuntimeError("{} vertex positions against {} weighted vertices — the two do not index alike".format(
            len(positions), modifier.get_num_vertices()))

    report = {}
    for index, position in enumerate(positions):
        distance, _, bone = min((distance_to_outline((position.x, position.y), outline), abs(position.z - lift), bone)
                                for bone, outline, lift in owners)
        if distance > SNAP_TOLERANCE:
            raise RuntimeError("vertex {} at {} sits {:.1f} away from every outline".format(index, position, distance))
        modifier.set_vertex_weights(index, {unreal.Name(bone): 1.0}, True)
        report[bone] = report.get(bone, 0) + 1

    modifier.commit_weights_to_skeletal_mesh()
    return report


def write_socket(mesh, name, bone, location):
    """The socket `name` on `bone` at `location` in that bone's space -> [bone, x, y] for the report.

    Rewritten in place when the socket exists, so a re-run never stacks a second one under the same name.
    """
    socket = mesh.find_socket(name)
    if socket is None:
        # The name is read-only from script: a socket goes in under the default name and is renamed in place,
        # reusing any left under that name, since nothing from script removes one.
        if mesh.find_socket(DEFAULT_SOCKET_NAME) is None:
            mesh.add_socket(unreal.SkeletalMeshSocket(outer=mesh), False)
        mesh.rename_socket(unreal.Name(DEFAULT_SOCKET_NAME), unreal.Name(name))
        socket = mesh.find_socket(name)
    socket.set_socket_parent(mesh, bone)
    socket.set_socket_local_transform(unreal.Transform(location))
    return [bone, round(location.x, 2), round(location.y, 2)]


def place_sockets(mesh, sides):
    """anim_socket_<n> on the root, FIRE_SOCKET_REACH ahead and its side's Y across."""
    report = {}
    for index, side in enumerate(sides):
        name = "anim_socket_{}".format(index)
        report[name] = write_socket(mesh, name, ROOT, unreal.Vector(FIRE_SOCKET_REACH, side, 0.0))
    return report


def keyhole_centre(generator):
    """The Square block's keyhole centre in world XY."""
    hole_x, hole_y, _ = generator["SQ_HOLE"]
    centre_x, centre_y, scale = generator["frame"]("SM_SquareBadge")
    return (centre_y - hole_y) * scale, (hole_x - centre_x) * scale


def extent(outline):
    """[min x, max x, min y, max y] of an outline, for reading how much room a clip has."""
    xs, ys = [p[0] for p in outline], [p[1] for p in outline]
    return [round(min(xs), 2), round(max(xs), 2), round(min(ys), 2), round(max(ys), 2)]


def rig_badge(generator, badge, setup):
    parts = world_parts(generator, "SM_{}Badge".format(badge))
    outlines = [part.outline for part in parts]
    part_names = setup["parts"]
    if len(parts) != len(part_names) + 1:
        raise RuntimeError("{} has {} parts, the rig names {} plus the body".format(
            badge, len(parts), len(part_names)))

    static_mesh = unreal.load_asset("{}/SM_{}Badge".format(FOLDER, badge))
    skeleton = get_or_create("SK_{}Badge".format(badge), unreal.Skeleton)
    mesh = get_or_create("SKM_{}Badge".format(badge), unreal.SkeletalMesh)

    bones = bone_table(part_names, parts)
    added = add_missing_bones(mesh, bones)
    if not unreal.get_default_object(unreal.GeoAnimBuilderUtil).rebuild_skeletal_mesh_from_static_mesh(
            mesh, static_mesh, skeleton,
            [unreal.Name(name) for name, _, _ in bones],
            [unreal.Name(parent) for _, parent, _ in bones],
            [unreal.Transform(unreal.Vector(*location), unreal.Rotator(), unreal.Vector(1.0, 1.0, 1.0))
             for _, _, location in bones]):
        raise RuntimeError("RebuildSkeletalMeshFromStaticMesh failed for {} — see the editor log".format(badge))

    part_outlines = dict(zip(part_names, outlines[1:]))
    binding = bind_vertices(mesh, [(bone, part.outline, part.lift)
                                   for bone, part in zip([BOTTOM] + part_names, parts)])
    sockets = place_sockets(mesh, setup["sockets"])
    if "keyhole_socket" in setup:
        hole_x, hole_y = keyhole_centre(generator)
        box_x, box_y = box_centre(outlines)
        sockets[setup["keyhole_socket"]] = write_socket(mesh, setup["keyhole_socket"], BOTTOM,
                                                        unreal.Vector(hole_x - box_x, hole_y - box_y, LAYER_LIFT))
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.EditorAssetLibrary.save_loaded_asset(skeleton)

    reference = APE.get_reference_pose(skeleton)
    return {"mesh": mesh.get_path_name(), "skeleton": skeleton.get_path_name(),
            "added": added, "binding": binding, "sockets": sockets,
            "bones": {str(bone): [round(v, 2) for v in (lambda t: (t.x, t.y, t.z))(
                APE.get_bone_pose(reference, str(bone), unreal.AnimPoseSpaces.WORLD).translation)]
                for bone in APE.get_bone_names(reference)},
            "extents": dict([(BOTTOM, extent(outlines[0]))]
                            + [(name, extent(outline)) for name, outline in part_outlines.items()])}


def main():
    result = {}
    try:
        generator = load_generator()
        for badge, setup in BADGES.items():
            result[badge] = rig_badge(generator, badge, setup)
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_saved_dir() + "class_badge_rig.json", "w") as f:
        json.dump(result, f, indent=2)


main()
