"""
Generates SM_StarBody: the star boss body as a compass star with a hollow heart.

Seen from the orthographic top-down camera it reads as eight long points with a short counter-point between each
pair, pierced through the middle by a star-shaped hole that repeats the outline eight points to eight. The material
is unlit and opaque, so a hole is the only interior detail the camera can see: solid star around hollow star, one
polygon at two energies.

The rig it goes on already exists (SK_Star) and every clip drives that rig's bones, so the outline is laid on it
rather than the other way round: the long points sit exactly where the tip bones do, and the counter-points where
the valley ring used to be.

The body is three stacked copies of the outline, bottom, middle and top, joined by walls — the middle copy is what
the clips that turn the `mid` bone twist out of the silhouette. Every copy of the outline and of the hole shares one
set of angles, so each cap is a ring of quads between an outer vertex and the hole vertex on the same ray.

Each long point is capped as a triangle of its own standing on the chord between its two valleys, never joined to
the hole: the clips turn the points against the body, and a triangle reaching from a tip down to the hole flips —
and is culled — as soon as that tip turns past its valley, where one on the chord takes a turn of about sixty
degrees.

Geometry lives here alone: this also emits, for every vertex, which copy it is on and whether it is the tip of a
long point, which is all AI/Python/Mesh/rig_star.py needs to weight it, and every triangle as three indices into
those vertices, for checking what a pose leaves facing the camera.

Run via mcp-unreal execute_script. Summary written to Saved/star_gen.json, spec to Saved/star_parts.json.
"""
import json
import math

import unreal

ASSET_NAME = "SM_StarBody"
FOLDER = "/Game/Art/Meshes/Boss"
PACKAGE = f"{FOLDER}/{ASSET_NAME}"
MATERIAL = "/Game/Characters/Meshes/Star/MAT_EnemyMat"
SLOT_NAME = "Color"
SPEC_FILE = "star_parts.json"

POINTS = 8
TIP_RADIUS = 70.71      # the old star's tips, where the tip bones sit
COUNTER_RADIUS = 54.12  # the short point between two long ones, where the old star's valleys sat
VALLEY_RADIUS = 42.0    # between a long point and a counter-point
HOLE_POINT_RADIUS = 24.0   # the hole's points, aimed at the long points
HOLE_VALLEY_RADIUS = 13.0  # its valleys, aimed at the counter-points

LAYERS = {"bottom": -50.0, "middle": 0.0, "top": 50.0}  # bottom first, then up: the walls join them in this order

# Per step of the outline in one point's worth of angle: (outer radius, is a tip). The hole's step lands on the
# same angle, which is what pairs the two outlines vertex for vertex.
OUTLINE_STEPS = [(TIP_RADIUS, True), (VALLEY_RADIUS, False), (COUNTER_RADIUS, False), (VALLEY_RADIUS, False)]


def step_angle(index):
    return 2.0 * math.pi * index / (POINTS * len(OUTLINE_STEPS))


def ray_hits_segment(angle, first, second):
    """Distance from the origin along the ray at `angle` to where it crosses the segment first-second."""
    dx, dy = math.cos(angle), math.sin(angle)
    ex, ey = second[0] - first[0], second[1] - first[1]
    return (first[0] * ey - first[1] * ex) / (dx * ey - dy * ex)


def hole_radius(index):
    """The hole's outline on the ray of outline step `index`: its points and valleys, and its straight edges
    between them wherever the outer outline has a vertex the hole does not."""
    half_point = len(OUTLINE_STEPS) // 2
    if index % half_point == 0:
        return HOLE_POINT_RADIUS if index % len(OUTLINE_STEPS) == 0 else HOLE_VALLEY_RADIUS

    before, after = index - index % half_point, index - index % half_point + half_point
    corners = [(hole_radius(corner) * math.cos(step_angle(corner)), hole_radius(corner) * math.sin(step_angle(corner)))
               for corner in (before, after)]
    return ray_hits_segment(step_angle(index), *corners)


def outlines():
    """[(angle, outer radius, hole radius, is a tip)] counter-clockwise from the first long point at +X."""
    return [(step_angle(index), OUTLINE_STEPS[index % len(OUTLINE_STEPS)][0], hole_radius(index),
             OUTLINE_STEPS[index % len(OUTLINE_STEPS)][1]) for index in range(POINTS * len(OUTLINE_STEPS))]


def signed_area(a, b, c):
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])


def build(rays):
    """Build the asset -> (asset, [(x, y, z, layer, is a tip)] per vertex, [(a, b, c)] per triangle into those)."""
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()

    group = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(group, SLOT_NAME)
    points = []
    triangles = []

    def tri(va, vb, vc):
        desc.create_triangle(group, [desc.create_vertex_instance(va[0]),
                                     desc.create_vertex_instance(vb[0]),
                                     desc.create_vertex_instance(vc[0])])
        triangles.append((va[3], vb[3], vc[3]))

    def ring(layer, radius_of, tip_of):
        """One copy of an outline on `layer` -> [(vertex id, x, y, point index)]."""
        created = []
        for angle, outer, hole, tip in rays:
            radius = radius_of(outer, hole)
            x, y = radius * math.cos(angle), radius * math.sin(angle)
            vertex = desc.create_vertex()
            desc.set_vertex_position(vertex, unreal.Vector(x, y, LAYERS[layer]))
            created.append((vertex, x, y, len(points)))
            points.append((x, y, LAYERS[layer], layer, tip_of(tip)))
        return created

    outer = {layer: ring(layer, lambda o, h: o, lambda tip: tip) for layer in LAYERS}
    hole = {layer: ring(layer, lambda o, h: h, lambda tip: False) for layer in LAYERS}

    count = len(rays)
    stack = list(LAYERS)
    for low, high in zip(stack, stack[1:]):
        for i in range(count):
            n = (i + 1) % count
            # The outer wall faces away from the centre, the hole's wall towards it.
            tri(outer[low][i], outer[low][n], outer[high][n])
            tri(outer[low][i], outer[high][n], outer[high][i])
            tri(hole[low][n], hole[low][i], hole[high][i])
            tri(hole[low][n], hole[high][i], hole[high][n])

    # Caps: a long point is its own triangle on its valleys' chord, with the body between that chord and the hole
    # fanned from the hole's point. Every other sector is a quad from the outline in to the hole, concave at a
    # valley, so split along whichever diagonal keeps both halves counter-clockwise.
    xy = lambda v: (v[1], v[2])
    tips = [tip for _, _, _, tip in rays]
    for layer, facing_up in ((stack[-1], True), (stack[0], False)):
        o, h = outer[layer], hole[layer]
        faces = []
        for i in range(count):
            p, n = (i - 1) % count, (i + 1) % count
            if tips[i]:
                faces += [(o[p], o[i], o[n]), (o[p], o[n], h[i]), (o[p], h[i], h[p]), (o[n], h[n], h[i])]
            elif not tips[n]:
                if signed_area(xy(o[i]), xy(o[n]), xy(h[n])) > 0.0 and signed_area(xy(o[i]), xy(h[n]), xy(h[i])) > 0.0:
                    faces += [(o[i], o[n], h[n]), (o[i], h[n], h[i])]
                else:
                    faces += [(o[i], o[n], h[i]), (o[n], h[n], h[i])]
        for a, b, c in faces:
            if facing_up:
                tri(a, b, c)
            else:
                tri(a, c, b)

    # Rewritten in place: deleting a loaded asset leaves its package unloadable for the whole session.
    asset = unreal.load_asset(PACKAGE) or unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, FOLDER, unreal.StaticMesh, None)

    asset.build_from_static_mesh_descriptions([desc], False, True)
    asset.set_editor_property("static_materials",
                              [unreal.StaticMaterial(material_interface=unreal.load_asset(MATERIAL),
                                                     material_slot_name=SLOT_NAME)])

    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset, points, triangles


def main():
    result = {}
    try:
        rays = outlines()
        asset, points, triangles = build(rays)
        with open(unreal.Paths.project_saved_dir() + SPEC_FILE, "w") as f:
            json.dump({"layers": LAYERS, "points": points, "triangles": triangles}, f)

        result["ok"] = True
        result["path"] = PACKAGE
        result["outline"] = [[round(math.degrees(angle), 2), outer, round(hole, 2), tip]
                             for angle, outer, hole, tip in rays[:len(OUTLINE_STEPS) + 1]]
        result["num_points"] = len(points)
        result["num_triangles"] = asset.get_num_triangles(0)
        result["num_vertices"] = asset.get_num_vertices(0)
        result["bounds"] = str(asset.get_bounds().box_extent)
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open(unreal.Paths.project_saved_dir() + "star_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
