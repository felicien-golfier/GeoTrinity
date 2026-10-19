"""
Generates SM_TriangleTurret: the Triangle class turret seen from above as a flat triangular body, its tip toward +X,
a round hub on its centre carrying a long cannon out of the tip, and a grey side cannon at each back corner.

Reference (top view, +X up): body tip at the front, hub on the centre (dark rim, light disc, dark ring, light disc,
dark dot), a dark rear block behind the hub, the cannon from the hub to a dark muzzle block carrying two thin stripes,
the cannon itself yellow with three dark bands, and two grey side cannons at the back corners, tilted outward, each
with a dark slot near its end.

The cannon, its rear block and the side cannons are prisms with a triangular section, apex up: from above they read
as plain bars, but when one rolls its three faces show. Slots: `Color` body, `Cannon` every light part, `CannonBand`
every dark part, `Legs` the grey side cannons.

The turret actor yaws as a whole toward its target (AGeoTurret::Tick), so +X is the firing direction. Side cannons sink
into the body, so no faces show inside.

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

BODY_TIP = 32.9  # forward of the hub centre
BODY_BACK = -19.8
BODY_HALF_BASE = 29.5
BODY_BOTTOM = 5.0
BODY_TOP = 13.0
HUB_SIDES = 32
# (slot, radius, top): stacked from the body top, each disc on the one before.
HUB_LAYERS = ((BAND_SLOT, 12.2, 16.0), (CANNON_SLOT, 11.0, 17.0), (BAND_SLOT, 8.0, 18.0), (CANNON_SLOT, 6.8, 19.0),
              (BAND_SLOT, 1.9, 20.0))
CANNON_RADIUS = 5.3  # circumradius of the triangular section of cannon, rear block and side cannons
CANNON_START = 12.2  # the hub's rim
# (slot, from, to) along the cannon, forward of the hub centre.
CANNON_SEGMENTS = ((CANNON_SLOT, 12.2, 22.4), (BAND_SLOT, 22.4, 25.8), (CANNON_SLOT, 25.8, 32.7),
                   (BAND_SLOT, 32.7, 36.1), (CANNON_SLOT, 36.1, 42.2), (BAND_SLOT, 42.2, 43.7),
                   (CANNON_SLOT, 43.7, 52.4))
REAR_BLOCK = (-17.9, -12.2)
MUZZLE_BLOCK = (52.4, 61.4)  # box, forward extent
MUZZLE_HALF_WIDTH = 7.9
MUZZLE_TOP = 22.0
MUZZLE_STRIPES = ((58.1, 59.1), (55.5, 56.4))  # forward extents of the two light stripes
MUZZLE_STRIPE_HALF_WIDTH = 5.9
MUZZLE_STRIPE_HEIGHT = 0.6
SIDE_CANNON_RADIUS = 5.5
SIDE_CANNON_HEIGHT = 5.0  # of the centre line above the floor
SIDE_CANNON_ANGLES = (110.0, 250.0)  # outward and slightly back, either side
SIDE_CANNON_SINK = 3.0  # inside the body, behind the corner
SIDE_CANNON_LENGTH = 17.0  # beyond the corner
SIDE_SLOT_RADIUS = 2.8
SIDE_SLOT_RANGE = (8.0, 13.0)  # along the side cannon, from the corner


def triangle(radius, apex_deg):
    """Three points, CCW seen from the axis the outline is extruded along."""
    return [(radius * math.cos(math.radians(apex_deg + 120.0 * i)),
             radius * math.sin(math.radians(apex_deg + 120.0 * i))) for i in range(3)]


def circle(radius):
    return [(radius * math.cos(2.0 * math.pi * i / HUB_SIDES), radius * math.sin(2.0 * math.pi * i / HUB_SIDES))
            for i in range(HUB_SIDES)]


def rectangle(x0, x1, half_width):
    return [(x0, -half_width), (x1, -half_width), (x1, half_width), (x0, half_width)]


def place_along(angle_deg, height, origin=(0.0, 0.0)):
    """(u, v, w) -> world for a prism along the horizontal direction `angle_deg` from `origin`, centre line at `height`.

    u runs sideways, v up, w outward: a cyclic permutation followed by a turn about Z, so winding is kept.
    """
    a = math.radians(angle_deg)
    c, s = math.cos(a), math.sin(a)
    return lambda u, v, w: (origin[0] + w * c - u * s, origin[1] + w * s + u * c, v + height)


def build_parts():
    """Every part is (slot, outline CCW in (u, v), w_bottom, w_top, place mapping (u, v, w) to world)."""
    flat = lambda u, v, w: (u, v, w)
    body = [(BODY_TIP, 0.0), (BODY_BACK, BODY_HALF_BASE), (BODY_BACK, -BODY_HALF_BASE)]
    parts = [(SOCLE_SLOT, body, BODY_BOTTOM, BODY_TOP, flat)]

    bottom = BODY_TOP
    for slot, radius, top in HUB_LAYERS:
        parts.append((slot, circle(radius), bottom, top, flat))
        bottom = top

    # The section's flat side rests on the body top: its lowest point is half the circumradius under the centre.
    cannon_height = BODY_TOP + CANNON_RADIUS * 0.5
    section = triangle(CANNON_RADIUS, 90.0)
    along_cannon = place_along(0.0, cannon_height)
    for slot, start, end in CANNON_SEGMENTS:
        parts.append((slot, section, start, end, along_cannon))
    parts.append((BAND_SLOT, section, REAR_BLOCK[0], REAR_BLOCK[1], along_cannon))

    parts.append((BAND_SLOT, rectangle(MUZZLE_BLOCK[0], MUZZLE_BLOCK[1], MUZZLE_HALF_WIDTH), BODY_TOP, MUZZLE_TOP,
                  flat))
    for start, end in MUZZLE_STRIPES:
        parts.append((CANNON_SLOT, rectangle(start, end, MUZZLE_STRIPE_HALF_WIDTH), MUZZLE_TOP,
                      MUZZLE_TOP + MUZZLE_STRIPE_HEIGHT, flat))

    for angle in SIDE_CANNON_ANGLES:
        corner = (BODY_BACK, BODY_HALF_BASE if math.sin(math.radians(angle)) > 0.0 else -BODY_HALF_BASE)
        parts.append((LEGS_SLOT, triangle(SIDE_CANNON_RADIUS, 90.0), -SIDE_CANNON_SINK, SIDE_CANNON_LENGTH,
                      place_along(angle, SIDE_CANNON_HEIGHT, corner)))
        ridge = SIDE_CANNON_HEIGHT + SIDE_CANNON_RADIUS
        parts.append((BAND_SLOT, triangle(SIDE_SLOT_RADIUS, 90.0), SIDE_SLOT_RANGE[0], SIDE_SLOT_RANGE[1],
                      place_along(angle, ridge, corner)))
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
