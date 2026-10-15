"""
Generates SM_HexMine: the hex boss mine as a flat hexagon with six hexagonal-section spikes, one on each
vertex, alternating long and short, and a small core hexagon on top in its own material slot.

Slot "Core" is the blinking red centre: give it a material that pulses, "Color" holds everything else.
The spikes are the turret cannon's shape (AI/Python/Mesh/generate_hex_turret_mesh.py) turned about Z, so they read
as six faces when they roll. Every part is its own convex prism sinking into the body, so no faces show inside.

Run via mcp-unreal execute_script. Summary written to AI/Output/hex_mine_gen.json.
"""
import json
import math

import unreal

ASSET_NAME = "SM_HexMine"
FOLDER = "/Game/Art/Meshes/Boss"
PACKAGE = f"{FOLDER}/{ASSET_NAME}"
BODY_SLOT = "Color"
CORE_SLOT = "Core"

BODY_RADIUS = 30.0
BODY_HEIGHT = 18.0
SPIKE_RADIUS = 6.0  # circumradius of a spike's hexagonal section
SPIKE_SINK = 10.0  # how far a spike's base sits inside the body
LONG_SPIKE_LENGTH = 26.0  # beyond the body's vertex
SHORT_SPIKE_LENGTH = 13.0
CORE_RADIUS = 15.0
CORE_BOTTOM = 14.0  # inside the body, so the core's bottom never shows
CORE_TOP = 22.0


def hexagon(radius, rotation_deg):
    """Six points, CCW seen from the axis the outline is extruded along."""
    rot = math.radians(rotation_deg)
    return [(radius * math.cos(rot + math.pi * i / 3.0),
             radius * math.sin(rot + math.pi * i / 3.0)) for i in range(6)]


def spike_place(angle_deg):
    """(u, v, w) -> world for a prism along the horizontal direction `angle_deg`, centred at mid body height.

    u runs sideways, v up, w outward: a cyclic permutation followed by a turn about Z, so winding is kept.
    """
    a = math.radians(angle_deg)
    c, s = math.cos(a), math.sin(a)
    return lambda u, v, w: (w * c - u * s, w * s + u * c, v + BODY_HEIGHT * 0.5)


def build_parts():
    """Every part is (slot, outline CCW in (u, v), w_bottom, w_top, place mapping (u, v, w) to world)."""
    parts = [(BODY_SLOT, hexagon(BODY_RADIUS, 0.0), 0.0, BODY_HEIGHT, lambda u, v, w: (u, v, w))]
    for i in range(6):
        length = LONG_SPIKE_LENGTH if i % 2 == 0 else SHORT_SPIKE_LENGTH
        parts.append((BODY_SLOT, hexagon(SPIKE_RADIUS, 0.0), BODY_RADIUS - SPIKE_SINK, BODY_RADIUS + length,
                      spike_place(60.0 * i)))
    parts.append((CORE_SLOT, hexagon(CORE_RADIUS, 0.0), CORE_BOTTOM, CORE_TOP, lambda u, v, w: (u, v, w)))
    return parts


def build(parts):
    """Build the asset from the parts."""
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()

    groups = {}
    for slot in (BODY_SLOT, CORE_SLOT):
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
                              [unreal.StaticMaterial(material_slot_name=BODY_SLOT),
                               unreal.StaticMaterial(material_slot_name=CORE_SLOT)])

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
    with open((unreal.Paths.project_dir() + "AI/Output/") + "hex_mine_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
