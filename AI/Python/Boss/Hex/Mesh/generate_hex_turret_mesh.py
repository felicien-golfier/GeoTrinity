"""
Generates SM_HexTurret: the hex boss turret as a plain flat hexagon with a cannon pointing out of its +X vertex.

The cannon is a hexagonal prism lying along X, so seen from the side or while it rolls it reads as six
faces, not a round tube. It is its own convex prism that touches nothing but sinks into the body, so it can
move alone once the mesh is rigged (same pattern as AI/Python/Boss/Hex/Mesh/rig_hex_boss.py).

The turret actor yaws as a whole toward its target (AGeoTurret::Tick), so +X is the firing direction.

Run via mcp-unreal execute_script. Summary written to AI/Output/hex_turret_gen.json.
"""
import json
import math

import unreal

mesh_toolkit_path = unreal.Paths.project_dir() + "AI/Python/Mesh/static_mesh_asset.py"
mesh_toolkit = {}
exec(compile(open(mesh_toolkit_path).read(), mesh_toolkit_path, "exec"), mesh_toolkit)

ASSET_NAME = "SM_HexTurret"
FOLDER = "/Game/Art/Meshes/Boss"
PACKAGE = f"{FOLDER}/{ASSET_NAME}"
SLOT_NAME = "Color"

BODY_RADIUS = 33.0
BODY_HEIGHT = 17.0
CANNON_RADIUS = 8.0  # circumradius of the cannon's hexagonal section
CANNON_LENGTH = 28.0  # beyond the body's front vertex
CANNON_SINK = 14.0  # how far the cannon's base sits inside the body


def hexagon(radius, rotation_deg):
    """Six points, CCW seen from the axis the outline is extruded along."""
    rot = math.radians(rotation_deg)
    return [(radius * math.cos(rot + math.pi * i / 3.0),
             radius * math.sin(rot + math.pi * i / 3.0)) for i in range(6)]


def build_parts():
    """Every part is (outline CCW in (u, v), w_bottom, w_top, place), place mapping (u, v, w) to world (x, y, z).

    The cannon's place is a cyclic permutation (u->Y, v->Z, w->X), so it keeps the winding of the body's identity.
    Its hexagon has a vertex on +Y and a flat on top, so the top view shows the full width.
    """
    return [
        (hexagon(BODY_RADIUS, 0.0), 0.0, BODY_HEIGHT, lambda u, v, w: (u, v, w)),
        (hexagon(CANNON_RADIUS, 0.0), BODY_RADIUS - CANNON_SINK, BODY_RADIUS + CANNON_LENGTH,
         lambda u, v, w: (w, u, v + BODY_HEIGHT * 0.5)),
    ]


def build(parts):
    """Build the asset from the parts."""
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()

    group = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(group, SLOT_NAME)

    def tri(va, vb, vc):
        # Wound CCW seen from outside; UE front faces are clockwise, so reverse here.
        desc.create_triangle(group, [desc.create_vertex_instance(vc),
                                     desc.create_vertex_instance(vb),
                                     desc.create_vertex_instance(va)])

    def extrude(outline, w0, w1, place):
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
            tri(bottom[i], bottom[n], top[n])
            tri(bottom[i], top[n], top[i])

        # Convex fans for the caps: top faces +w, bottom reversed.
        for i in range(1, count - 1):
            tri(top[0], top[i], top[i + 1])
            tri(bottom[0], bottom[i + 1], bottom[i])

    for outline, w0, w1, place in parts:
        extrude(outline, w0, w1, place)

    asset = mesh_toolkit["save_static_mesh"](FOLDER, ASSET_NAME, desc,
                                             [unreal.StaticMaterial(material_slot_name=SLOT_NAME)])
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
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "hex_turret_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
