"""
Generates SM_Bomb: a round bomb Static Mesh sized to replace AGeoPillar.

Body is a cylinder of radius 100 fitting the pillar's 100/100 capsule, so it drops
in without touching collision. Read from the orthographic top-down camera it is a
perfect circle — the silhouette that makes it recognisable is the four fins, which
sit on a short tail stack ABOVE the body's flat top and inside its radius, so they
read as a cross laid over the circle.

Geometry is built from generic prisms: an outline lying in a plane, swept along a
direction. Winding is resolved from the outline's Newell normal, so outlines may
be listed in either order. Members overlap where they meet — buried faces are
interior and never visible, which avoids any CSG.

Run via mcp-unreal execute_script. Summary written to Saved/bomb_gen.json.
"""
import unreal, json, math

ASSET_NAME = "SM_Bomb"
FOLDER     = "/Game/Art/Meshes/Bomb"
PACKAGE    = f"{FOLDER}/{ASSET_NAME}"
SLOT_NAME  = "Color"

R_BODY     = 90.0
BODY_SEGS  = 24   # enough to read as a clean circle from the top
BODY_Z0    = -100.0
BODY_Z1    = 80.0   # flat top the tail rises out of

TAIL_R0    = 34.0   # tail cone: wide where it meets the body, narrow at the top
TAIL_R1    = 24.0
TAIL_Z0    = 80.0
TAIL_Z1    = 168.0
TAIL_SEGS  = 12

FIN_COUNT  = 4
FIN_THICK  = 12.0
# Fin profile in the (radius, Z) plane. Swept back: tall and narrow at the tail,
# widening outward, with the trailing edge raked up so it reads as a fin, not a slab.
# Reaches past R_BODY so the fins overhang the cylinder when seen from the top.
FIN_R_OUTER = R_BODY * 1.1
FIN_PROFILE = [
    (20.0,        92.0),
    (FIN_R_OUTER, 118.0),
    (FIN_R_OUTER, 176.0),
    (20.0,        176.0),
]


def build():
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()

    group = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(group, SLOT_NAME)

    def vert(p):
        v = desc.create_vertex()
        desc.set_vertex_position(v, unreal.Vector(p[0], p[1], p[2]))
        return v

    def tri(va, vb, vc):
        # Outlines below are wound CCW seen from outside; UE front-faces are
        # clockwise, so the order is reversed here rather than at every call site.
        desc.create_triangle(group, [desc.create_vertex_instance(vc),
                                     desc.create_vertex_instance(vb),
                                     desc.create_vertex_instance(va)])

    def newell(points):
        n = [0.0, 0.0, 0.0]
        for i, a in enumerate(points):
            b = points[(i + 1) % len(points)]
            n[0] += (a[1] - b[1]) * (a[2] + b[2])
            n[1] += (a[2] - b[2]) * (a[0] + b[0])
            n[2] += (a[0] - b[0]) * (a[1] + b[1])
        return n

    def prism(outline, direction):
        """Sweep a planar convex outline along `direction`; caps both ends."""
        n = newell(outline)
        if sum(n[i] * direction[i] for i in range(3)) < 0.0:
            outline = list(reversed(outline))

        far = [vert(p) for p in outline]
        near = [vert([p[i] - direction[i] for i in range(3)]) for p in outline]

        count = len(outline)
        for i in range(count):
            j = (i + 1) % count
            tri(near[i], near[j], far[j])
            tri(near[i], far[j], far[i])
        for i in range(1, count - 1):
            tri(far[0], far[i], far[i + 1])
            tri(near[0], near[i + 1], near[i])

    def ring_of(radius, z, segs):
        return [vert([radius * math.cos(2.0 * math.pi * s / segs),
                      radius * math.sin(2.0 * math.pi * s / segs), z])
                for s in range(segs)]

    def tube(low, high, segs, cap_top, cap_bottom):
        for s in range(segs):
            n = (s + 1) % segs
            tri(low[s], low[n], high[n])
            tri(low[s], high[n], high[s])
        for s in range(1, segs - 1):
            if cap_top:
                tri(high[0], high[s], high[s + 1])
            if cap_bottom:
                tri(low[0], low[s + 1], low[s])

    # --- Body: flat-topped cylinder. ---
    tube(ring_of(R_BODY, BODY_Z0, BODY_SEGS), ring_of(R_BODY, BODY_Z1, BODY_SEGS),
         BODY_SEGS, True, True)

    # --- Tail: truncated cone rising out of the top of the body. ---
    tube(ring_of(TAIL_R0, TAIL_Z0, TAIL_SEGS), ring_of(TAIL_R1, TAIL_Z1, TAIL_SEGS),
         TAIL_SEGS, True, False)

    # --- Fins: swept radially, thickness across the radial direction. ---
    for i in range(FIN_COUNT):
        angle = 2.0 * math.pi * i / FIN_COUNT
        radial = (math.cos(angle), math.sin(angle), 0.0)
        across = (-math.sin(angle), math.cos(angle), 0.0)
        outline = [[radial[0] * r + across[0] * FIN_THICK * 0.5,
                    radial[1] * r + across[1] * FIN_THICK * 0.5, z]
                   for r, z in FIN_PROFILE]
        prism(outline, [-across[k] * FIN_THICK for k in range(3)])

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
    with open(unreal.Paths.project_saved_dir() + "bomb_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
