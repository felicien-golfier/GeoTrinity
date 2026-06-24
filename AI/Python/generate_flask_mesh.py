"""
Generates SM_Flask: a low-poly faceted flask/potion Static Mesh asset.

Silhouette (bottom -> top): flat base, bulbous faceted body, a short pinch into a
narrow neck, then a flared lip. Faceted (flat-shaded) to read like the low-poly
reference. Single material slot ("Color") so a dynamic material instance can tint
the whole mesh from one vector parameter — matching AGeoBuffPickup's UpdateColor().

Run via mcp-unreal execute_script. Writes the result summary to Saved/flask_gen.json.
"""
import unreal, json, math

ASSET_NAME = "SM_Flask"
FOLDER     = "/Game/Art/Meshes/Flask"
PACKAGE    = f"{FOLDER}/{ASSET_NAME}"
SLOT_NAME  = "Color"

# Radial facets. Low number => chunky, faceted reference look.
SIDES = 8

# Profile as (radius, height) rings from bottom to top, in cm.
# Tuned for a squat potion flask: wide body, pinched neck, small flared lip.
PROFILE = [
    (0.0,   0.0),    # 0 base center (cap)
    (14.0,  0.0),    # 1 base rim
    (20.0,  10.0),   # 2 body widest-low
    (22.0,  26.0),   # 3 body widest
    (18.0,  40.0),   # 4 shoulder
    (8.0,   50.0),   # 5 neck pinch
    (7.0,   62.0),   # 6 neck
    (11.0,  70.0),   # 7 lip flare
    (10.0,  72.0),   # 8 lip top rim
    (0.0,   72.0),   # 9 top center (cap)
]


def build():
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()

    group = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(group, SLOT_NAME)

    # Pre-create the ring vertices (skip the two cap centers; handled separately).
    # ring_verts[i] = list of vertex ids around profile ring i (None for cap rings).
    ring_verts = []
    for radius, height in PROFILE:
        if radius <= 0.0:
            ring_verts.append(None)
            continue
        verts = []
        for s in range(SIDES):
            ang = (2.0 * math.pi * s) / SIDES
            v = desc.create_vertex()
            desc.set_vertex_position(
                v, unreal.Vector(radius * math.cos(ang), radius * math.sin(ang), height))
            verts.append(v)
        ring_verts.append(verts)

    def tri(va, vb, vc):
        # create_triangle makes the encapsulating polygon and edges automatically.
        vis = [desc.create_vertex_instance(va), desc.create_vertex_instance(vb),
               desc.create_vertex_instance(vc)]
        desc.create_triangle(group, vis)

    def quad(a0, a1, b1, b0):
        # One flat-shaded quad as two triangles (CCW seen from outside).
        tri(a0, a1, b1)
        tri(a0, b1, b0)

    def fan(center_vid, ring, up):
        # Triangle fan from a cap center to a ring. `up` flips winding for top cap.
        for s in range(SIDES):
            n = (s + 1) % SIDES
            if up:
                tri(center_vid, ring[s], ring[n])
            else:
                tri(center_vid, ring[n], ring[s])

    # Bottom cap (profile index 0 center -> ring 1), faces down.
    base_center = desc.create_vertex()
    desc.set_vertex_position(base_center, unreal.Vector(0, 0, PROFILE[0][1]))
    fan(base_center, ring_verts[1], up=False)

    # Body: connect each adjacent pair of solid rings.
    solid = [i for i, rv in enumerate(ring_verts) if rv is not None]
    for lo, hi in zip(solid, solid[1:]):
        a, b = ring_verts[lo], ring_verts[hi]
        for s in range(SIDES):
            n = (s + 1) % SIDES
            quad(a[s], a[n], b[n], b[s])

    # Top cap (lip top rim -> top center), faces up.
    top_center = desc.create_vertex()
    desc.set_vertex_position(top_center, unreal.Vector(0, 0, PROFILE[-1][1]))
    fan(top_center, ring_verts[solid[-1]], up=True)

    # Create the asset package and build the render mesh.
    factory = unreal.AssetToolsHelpers.get_asset_tools()
    if unreal.EditorAssetLibrary.does_asset_exist(PACKAGE):
        unreal.EditorAssetLibrary.delete_asset(PACKAGE)

    asset = factory.create_asset(ASSET_NAME, FOLDER, unreal.StaticMesh, None)
    # build_simple_collision=False, fast_build=True
    asset.build_from_static_mesh_descriptions([desc], False, True)

    # Ensure one material slot named SLOT_NAME exists.
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
        result["num_sections"] = asset.get_num_sections(0)
        result["bounds"] = str(asset.get_bounds().box_extent)
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    p = unreal.Paths.project_saved_dir() + "flask_gen.json"
    with open(p, "w") as f:
        json.dump(result, f, indent=2)


main()
