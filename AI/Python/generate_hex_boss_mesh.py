"""
Generates SM_HexBossFrame: a boss-scale hexagonal architectural frame Static Mesh.

Seen from the orthographic top-down camera it reads as a hexagon built from
structure rather than a solid slab: an outer hex ring, a counter-rotated inner
hex ring, a raised hexagonal core, and radial spokes / diagonal struts tying
them together. The gaps between those members are what you see through.

Everything is composed from convex polygonal prisms (extrude a 2D outline
between two Z heights). Members deliberately overlap where they meet — the
buried faces are interior and never visible, which avoids any CSG.

Player capsule is 50cm, so R_OUT=400 makes the boss ~8x a player across.

Run via mcp-unreal execute_script. Summary written to Saved/hex_boss_gen.json.
"""
import unreal, json, math

ASSET_NAME = "SM_HexBossFrame"
FOLDER     = "/Game/Art/Meshes/Boss"
PACKAGE    = f"{FOLDER}/{ASSET_NAME}"
SLOT_NAME  = "Color"

R_OUT       = 400.0   # outer hex circumradius
RING_THICK  = 55.0
R_IN        = 250.0   # inner (counter-rotated) hex circumradius
IN_THICK    = 40.0
R_CORE      = 110.0

Z_OUT_RING  = 140.0
Z_IN_RING   = 110.0
Z_CORE      = 200.0
Z_SPOKE     = 110.0
Z_STRUT     = 100.0


def hexagon(radius, rotation_deg):
    """Six points, CCW seen from +Z."""
    rot = math.radians(rotation_deg)
    return [(radius * math.cos(rot + math.pi * i / 3.0),
             radius * math.sin(rot + math.pi * i / 3.0)) for i in range(6)]


def bar(angle_deg, r_start, r_end, half_width):
    """Rectangle running radially outward along `angle_deg`, CCW seen from +Z."""
    a = math.radians(angle_deg)
    ux, uy = math.cos(a), math.sin(a)
    px, py = -uy, ux  # left-hand perpendicular
    def p(r, s):
        return (ux * r + px * s * half_width, uy * r + py * s * half_width)
    return [p(r_start, -1), p(r_end, -1), p(r_end, 1), p(r_start, 1)]


def ring_quads(radius_outer, thickness, rotation_deg):
    """A hex annulus as six trapezoid outlines (one per hex edge)."""
    outer = hexagon(radius_outer, rotation_deg)
    inner = hexagon(radius_outer - thickness, rotation_deg)
    quads = []
    for i in range(6):
        n = (i + 1) % 6
        quads.append([outer[i], outer[n], inner[n], inner[i]])
    return quads


def build_parts():
    """Every part is (convex CCW outline in XY, z_bottom, z_top)."""
    parts = []

    for quad in ring_quads(R_OUT, RING_THICK, 0.0):
        parts.append((quad, 0.0, Z_OUT_RING))

    for quad in ring_quads(R_IN, IN_THICK, 30.0):
        parts.append((quad, 0.0, Z_IN_RING))

    parts.append((hexagon(R_CORE, 0.0), 0.0, Z_CORE))

    # Spokes: core -> outer ring, through the inner ring, on the hex vertices.
    for i in range(6):
        parts.append((bar(i * 60.0, R_CORE - 15.0, R_OUT - RING_THICK + 15.0, 22.5),
                      0.0, Z_SPOKE))

    # Diagonal struts: inner ring -> outer ring, offset 30 deg from the spokes,
    # so each sector is split into two triangular see-through openings.
    for i in range(6):
        parts.append((bar(30.0 + i * 60.0, R_IN - IN_THICK, R_OUT * math.cos(math.radians(30.0)), 17.5),
                      0.0, Z_STRUT))

    return parts


def build():
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()

    group = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(group, SLOT_NAME)

    def tri(va, vb, vc):
        desc.create_triangle(group, [desc.create_vertex_instance(va),
                                     desc.create_vertex_instance(vb),
                                     desc.create_vertex_instance(vc)])

    def extrude(outline, z0, z1):
        bottom, top = [], []
        for x, y in outline:
            vb = desc.create_vertex()
            desc.set_vertex_position(vb, unreal.Vector(x, y, z0))
            bottom.append(vb)
            vt = desc.create_vertex()
            desc.set_vertex_position(vt, unreal.Vector(x, y, z1))
            top.append(vt)

        count = len(outline)
        for i in range(count):
            n = (i + 1) % count
            tri(bottom[i], bottom[n], top[n])
            tri(bottom[i], top[n], top[i])

        # Convex fans for the caps: top faces +Z, bottom reversed.
        for i in range(1, count - 1):
            tri(top[0], top[i], top[i + 1])
            tri(bottom[0], bottom[i + 1], bottom[i])

    for outline, z0, z1 in build_parts():
        extrude(outline, z0, z1)

    if unreal.EditorAssetLibrary.does_asset_exist(PACKAGE):
        unreal.EditorAssetLibrary.delete_asset(PACKAGE)

    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, FOLDER, unreal.StaticMesh, None)
    asset.build_from_static_mesh_descriptions([desc], False, True)
    asset.set_editor_property("static_materials",
                              [unreal.StaticMaterial(material_slot_name=SLOT_NAME)])

    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def main():
    result = {}
    try:
        asset = build()
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
    with open(unreal.Paths.project_saved_dir() + "hex_boss_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
