"""
Generates SM_HexBossFrame: the hex boss body as three independent concentric hexagons.

Seen from the orthographic top-down camera it reads as three nested hexagons — an outer ring, a
counter-rotated middle ring and a solid core — each fringed with a spike on every face. Nothing joins
the three, so each is free to turn and grow on its own bone, and every spike on its own.

Everything is composed from convex polygonal prisms (extrude a 2D outline between two Z heights).
Spikes deliberately sink into the hexagon they grow from — the buried faces are interior and never
visible, which avoids any CSG.

Geometry lives here alone: this also emits the rig that AI/Python/rig_hex_boss.py builds, as the bone
every part hangs off, where that bone sits, and the position of every vertex it drives. The rig then
needs to know nothing about hexagons.

Player capsule is 50cm, so an outer radius of 400 makes the boss ~8x a player across.

Run via mcp-unreal execute_script. Summary written to Saved/hex_boss_gen.json, rig to
Saved/hex_boss_parts.json.
"""
import collections
import json
import math

import unreal

ASSET_NAME = "SM_HexBossFrame"
FOLDER = "/Game/Art/Meshes/Boss"
PACKAGE = f"{FOLDER}/{ASSET_NAME}"
SLOT_NAME = "Color"
SPEC_FILE = "hex_boss_parts.json"
ROOT_BONE = "Root"

SPIKE_SINK = 12.0  # how deep a spike's base sits inside the hexagon it grows from

Hexagon = collections.namedtuple("Hexagon", "radius thickness rotation height spike_length spike_half_base")

# Keyed by the bone that drives it. A thickness of 0 makes a solid hexagon.
HEXAGONS = {
    "HexOuter": Hexagon(400.0, 55.0, 0.0, 140.0, 120.0, 30.0),
    "HexMid": Hexagon(250.0, 45.0, 30.0, 110.0, 90.0, 22.0),
    "HexCore": Hexagon(110.0, 0.0, 0.0, 200.0, 40.0, 14.0),
}


def hexagon(radius, rotation_deg):
    """Six points, CCW seen from +Z."""
    rot = math.radians(rotation_deg)
    return [(radius * math.cos(rot + math.pi * i / 3.0),
             radius * math.sin(rot + math.pi * i / 3.0)) for i in range(6)]


def ring_quads(radius_outer, thickness, rotation_deg):
    """A hex annulus as six trapezoid outlines (one per hex edge)."""
    outer = hexagon(radius_outer, rotation_deg)
    inner = hexagon(radius_outer - thickness, rotation_deg)
    quads = []
    for i in range(6):
        n = (i + 1) % 6
        quads.append([outer[i], outer[n], inner[n], inner[i]])
    return quads


def spike(angle_deg, base_radius, tip_radius, half_base):
    """An isosceles triangle pointing outward along `angle_deg`, CCW seen from +Z."""
    a = math.radians(angle_deg)
    ux, uy = math.cos(a), math.sin(a)
    px, py = -uy, ux  # left-hand perpendicular
    return [(ux * base_radius - px * half_base, uy * base_radius - py * half_base),
            (ux * tip_radius, uy * tip_radius),
            (ux * base_radius + px * half_base, uy * base_radius + py * half_base)]


def apothem(shape):
    """Centre to the middle of a face — where that face's spike stands."""
    return shape.radius * math.cos(math.radians(30.0))


def spike_faces(name, shape):
    """(bone name, outward angle in degrees) for each of the six faces of the hexagon `name` drives."""
    return [("{}_Spike_{}".format(name, i), shape.rotation + 30.0 + i * 60.0) for i in range(6)]


def build_parts():
    """Every part is (bone name, convex CCW outline in XY, z_bottom, z_top)."""
    parts = []
    for name, shape in HEXAGONS.items():
        if shape.thickness > 0.0:
            for quad in ring_quads(shape.radius, shape.thickness, shape.rotation):
                parts.append((name, quad, 0.0, shape.height))
        else:
            parts.append((name, hexagon(shape.radius, shape.rotation), 0.0, shape.height))

        # One spike per face, standing on the middle of that face and pointing straight out of it.
        for bone, angle in spike_faces(name, shape):
            parts.append((bone, spike(angle, apothem(shape) - SPIKE_SINK, apothem(shape) + shape.spike_length,
                                      shape.spike_half_base), 0.0, shape.height))
    return parts


def bone_table():
    """The rig as (bone, parent, location, yaw), each placed in its parent's space, parents before children.

    That order and the root coming first is what FReferenceSkeleton requires of the hierarchy it is handed.

    A hexagon bone sits on the axis at its hexagon's mid-height: on the axis so a spin turns the hexagon about
    the boss's centre and a scale grows it around that centre instead of sliding it sideways, at its own height
    so three otherwise coincident bones stay apart and pickable in the viewport.

    A spike bone sits at the centre of its own base — buried SPIKE_SINK deep inside solid hexagon — and is yawed
    to face outward. Scaling it to zero folds the spike into that solid geometry rather than dragging it to the
    middle, and scaling X alone slides the blade out of the face and back in. It hangs off its own hexagon, so a
    spike rides that hexagon's spin and keeps pointing out of the face it grew from.
    """
    bones = [(ROOT_BONE, "None", (0.0, 0.0, 0.0), 0.0)]
    for name, shape in HEXAGONS.items():
        bones.append((name, ROOT_BONE, (0.0, 0.0, shape.height * 0.5), 0.0))
        for bone, angle in spike_faces(name, shape):
            base = apothem(shape) - SPIKE_SINK
            bones.append((bone, name, (base * math.cos(math.radians(angle)),
                                       base * math.sin(math.radians(angle)), 0.0), angle))
    return bones


def build(parts):
    """Build the asset -> (asset, [(x, y, z, bone)] for every vertex it was built from)."""
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()

    group = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(group, SLOT_NAME)
    points = []

    def tri(va, vb, vc):
        desc.create_triangle(group, [desc.create_vertex_instance(va),
                                     desc.create_vertex_instance(vb),
                                     desc.create_vertex_instance(vc)])

    def extrude(bone, outline, z0, z1):
        bottom, top = [], []
        for x, y in outline:
            vb = desc.create_vertex()
            desc.set_vertex_position(vb, unreal.Vector(x, y, z0))
            bottom.append(vb)
            vt = desc.create_vertex()
            desc.set_vertex_position(vt, unreal.Vector(x, y, z1))
            top.append(vt)
            points.append((x, y, z0, bone))
            points.append((x, y, z1, bone))

        count = len(outline)
        for i in range(count):
            n = (i + 1) % count
            tri(bottom[i], bottom[n], top[n])
            tri(bottom[i], top[n], top[i])

        # Convex fans for the caps: top faces +Z, bottom reversed.
        for i in range(1, count - 1):
            tri(top[0], top[i], top[i + 1])
            tri(bottom[0], bottom[i + 1], bottom[i])

    for bone, outline, z0, z1 in parts:
        extrude(bone, outline, z0, z1)

    # Rewritten in place: deleting a loaded asset leaves its package unloadable for the whole session.
    if unreal.EditorAssetLibrary.does_asset_exist(PACKAGE):
        asset = unreal.load_asset(PACKAGE)
    else:
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME, FOLDER, unreal.StaticMesh, None)

    asset.build_from_static_mesh_descriptions([desc], False, True)
    asset.set_editor_property("static_materials",
                              [unreal.StaticMaterial(material_slot_name=SLOT_NAME)])

    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset, points


def main():
    result = {}
    try:
        asset, points = build(build_parts())
        bones = bone_table()
        with open(unreal.Paths.project_saved_dir() + SPEC_FILE, "w") as f:
            json.dump({"bones": bones, "points": points}, f)

        result["ok"] = True
        result["path"] = PACKAGE
        result["num_bones"] = len(bones)
        result["num_points"] = len(points)
        result["num_triangles"] = asset.get_num_triangles(0)
        result["num_vertices"] = asset.get_num_vertices(0)
        result["bounds"] = str(asset.get_bounds().box_extent)
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_saved_dir() + "hex_boss_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
