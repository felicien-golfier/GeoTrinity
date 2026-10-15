"""
Generates SM_TriangleTurret: the Triangle class turret as a flat triangular socle, a flat edge toward +X, standing
on three low legs, with a cannon lying on the socle and out of its front edge.

Seen from the top-down camera the legs are three bars lying on the floor, one out of each tip of the socle: two
at the front corners with the cannon between them, one at the back. The cannon starts above the socle's centre,
so it is seen on the socle too. Legs and cannon have a triangular section, so a rolling cannon reads as three faces.

The cannon is a row of prisms alternating the `Cannon` and `CannonBand` slots: four thin regular stripes in colour
alone, no relief. `Legs` is a slot of its own so they can be tinted apart from the socle (`Color`).

The turret actor yaws as a whole toward its target (AGeoTurret::Tick), so +X is the firing direction. Legs sink
into the socle, so no faces show inside.

Run via mcp-unreal execute_script. Summary written to AI/Output/triangle_turret_gen.json.
"""
import json
import math

import unreal

ASSET_NAME = "SM_TriangleTurret"
FOLDER = "/Game/Art/Meshes/Boss"
PACKAGE = f"{FOLDER}/{ASSET_NAME}"
SOCLE_SLOT = "Color"
LEGS_SLOT = "Legs"
CANNON_SLOT = "Cannon"
BAND_SLOT = "CannonBand"
SLOTS = (SOCLE_SLOT, LEGS_SLOT, CANNON_SLOT, BAND_SLOT)

SOCLE_RADIUS = 34.0  # centre to tip
SOCLE_BOTTOM = 5.0
SOCLE_TOP = 13.0
LEG_RADIUS = 6.5  # circumradius of a leg's triangular section
LEG_HEIGHT = 5.0  # of the leg's centre line above the floor
LEG_LENGTH = 6.0  # beyond its socle tip
SINK = 8.0  # how far a leg sits inside the socle
CANNON_RADIUS = 3.5
CANNON_LENGTH = 30.0  # beyond the middle of the front edge
CANNON_BAND_COUNT = 4  # evenly spaced along the cannon, a gap of `Cannon` at both ends
CANNON_BAND_WIDTH = 3.25  # along the cannon


def triangle(radius, apex_deg):
    """Three points, CCW seen from the axis the outline is extruded along."""
    return [(radius * math.cos(math.radians(apex_deg + 120.0 * i)),
             radius * math.sin(math.radians(apex_deg + 120.0 * i))) for i in range(3)]


def place_along(angle_deg, height):
    """(u, v, w) -> world for a prism along the horizontal direction `angle_deg`, centre line at `height`.

    u runs sideways, v up, w outward: a cyclic permutation followed by a turn about Z, so winding is kept.
    """
    a = math.radians(angle_deg)
    c, s = math.cos(a), math.sin(a)
    return lambda u, v, w: (w * c - u * s, w * s + u * c, v + height)


def build_parts():
    """Every part is (slot, outline CCW in (u, v), w_bottom, w_top, place mapping (u, v, w) to world)."""
    front_edge = SOCLE_RADIUS * 0.5  # centre to the middle of an edge
    parts = [(SOCLE_SLOT, triangle(SOCLE_RADIUS, 180.0), SOCLE_BOTTOM, SOCLE_TOP, lambda u, v, w: (u, v, w))]
    for angle in (60.0, 180.0, 300.0):
        parts.append((LEGS_SLOT, triangle(LEG_RADIUS, 90.0), SOCLE_RADIUS - SINK, SOCLE_RADIUS + LEG_LENGTH,
                      place_along(angle, LEG_HEIGHT)))

    # The section's flat side rests on the socle top: its lowest point is half the circumradius under the centre.
    cannon_height = SOCLE_TOP + CANNON_RADIUS * 0.5
    cannon_end = front_edge + CANNON_LENGTH
    gap = (cannon_end - CANNON_BAND_COUNT * CANNON_BAND_WIDTH) / (CANNON_BAND_COUNT + 1)
    start = 0.0
    for i in range(2 * CANNON_BAND_COUNT + 1):
        length = CANNON_BAND_WIDTH if i % 2 == 1 else gap
        parts.append((BAND_SLOT if i % 2 == 1 else CANNON_SLOT, triangle(CANNON_RADIUS, 90.0), start,
                      start + length, place_along(0.0, cannon_height)))
        start += length
    return parts


def build(parts):
    """Build the asset from the parts."""
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()

    groups = {}
    for slot in SLOTS:
        groups[slot] = desc.create_polygon_group()
        desc.set_polygon_group_material_slot_name(groups[slot], slot)

    def tri(group, va, vb, vc):
        # Wound CCW seen from outside; UE front faces are clockwise, so reverse here.
        desc.create_triangle(group, [desc.create_vertex_instance(vc),
                                     desc.create_vertex_instance(vb),
                                     desc.create_vertex_instance(va)])

    def extrude(slot, outline, w0, w1, place):
        group = groups[slot]
        bottom, top = [], []
        for u, v in outline:
            vb = desc.create_vertex()
            desc.set_vertex_position(vb, unreal.Vector(*place(u, v, w0)))
            bottom.append(vb)
            vt = desc.create_vertex()
            desc.set_vertex_position(vt, unreal.Vector(*place(u, v, w1)))
            top.append(vt)

        count = len(outline)
        for i in range(count):
            n = (i + 1) % count
            tri(group, bottom[i], bottom[n], top[n])
            tri(group, bottom[i], top[n], top[i])

        # Convex fans for the caps: top faces +w, bottom reversed.
        for i in range(1, count - 1):
            tri(group, top[0], top[i], top[i + 1])
            tri(group, bottom[0], bottom[i + 1], bottom[i])

    for slot, outline, w0, w1, place in parts:
        extrude(slot, outline, w0, w1, place)

    # Rewritten in place: deleting a loaded asset leaves its package unloadable for the whole session.
    if unreal.EditorAssetLibrary.does_asset_exist(PACKAGE):
        asset = unreal.load_asset(PACKAGE)
        asset.get_num_vertices(0)
    else:
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME, FOLDER, unreal.StaticMesh, None)

    asset.build_from_static_mesh_descriptions([desc], False, True)
    asset.set_editor_property("static_materials",
                              [unreal.StaticMaterial(material_slot_name=slot) for slot in SLOTS])

    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def main():
    result = {}
    try:
        asset = build(build_parts())
        result["ok"] = True
        result["path"] = PACKAGE
        result["num_triangles"] = asset.get_num_triangles(0)
        result["num_vertices"] = asset.get_num_vertices(0)
        result["bounds"] = str(asset.get_bounds().box_extent)
        result["slots"] = [str(m.material_slot_name) for m in asset.get_editor_property("static_materials")]
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "triangle_turret_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
