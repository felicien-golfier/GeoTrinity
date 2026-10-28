"""
Generates the 3D gems: SM_Gem_Chip / Cut / Prism / Core, one solid per tier (octahedron, dodecahedron, icosahedron,
diamond), and SM_Shard_A / B / C, irregular crystal fragments (rules: Source/GeoTrinity/Public/Gem/CLAUDE.md,
"3D gems", "Shards").

Every mesh is the convex hull of a point set (coplanar points merge into one face), flat faces, slot "Gem",
centred on the origin (bounding box) so a drop's Z is its centre. Gem size grows with the tier: RADIUS is the
circumradius per tier. Each gem also carries its halo in slot "Halo": a smooth sphere of HALO_SCALE times the radius
around the stone, drawn as a soft additive glow by M_GemHalo (make_gem_materials.py); a sphere, so the tumble never
shows on it. Shards have no halo.

Edge distances ride in UV0 for the material to draw the edges lit (Gem/CLAUDE.md, "3D gems"): per triangle corner,
UV0 = (distance to the triangle's first real edge, to its second), FAR when it has fewer. A real edge is a side that is
an edge of the solid (not a spoke to a face centre), so only the solid's own edges light up. Every face is split around
its centre, one real edge per triangle.

Run via mcp-unreal execute_script or -run=pythonscript. Summary written to AI/Output/gem_meshes_gen.json.
"""
import itertools
import json
import math

import unreal

mesh_toolkit_path = unreal.Paths.project_dir() + "AI/Python/Mesh/static_mesh_asset.py"
mesh_toolkit = {}
exec(compile(open(mesh_toolkit_path).read(), mesh_toolkit_path, "exec"), mesh_toolkit)

FOLDER = "/Game/Art/Meshes/Gem"
SLOT = "Gem"
HALO_SLOT = "Halo"

RADIUS = {"Chip": 25.0, "Cut": 28.0, "Prism": 35.0, "Core": 44.0}  # circumradius per tier
PHI = (1.0 + math.sqrt(5.0)) / 2.0
HALO_SCALE = 1.7  # halo sphere radius, in gem radii
HALO_SUBDIVISIONS = 2  # icosahedron split this many times: 320 triangles

# Core diamond (low-poly brilliant cut), in girdle radii: octagonal girdle, table turned half a step on top, culet below.
DIAMOND_SIDES = 8
DIAMOND_TABLE = 0.55  # table radius
DIAMOND_CROWN = 0.32  # table height above the girdle
DIAMOND_PAVILION = 0.86  # culet depth below the girdle

SHARD_SCALE = 0.5  # one shard is one Forge unit and a kill throws many: each stays under a Chip
# Hand-listed convex fragments. A tall spike, a tilted block, a squat chunk.
SHARDS = {
    "A": [(14, -9, -22), (-11, -13, -20), (-15, 10, -24), (10, 15, -19), (3, -2, 48)],
    "B": [(-18, -14, -12), (16, -17, -14), (19, 12, -10), (-14, 16, -16),
          (-10, -11, 17), (21, -6, 13), (14, 14, 19), (-17, 12, 11)],
    "C": [(-24, -10, -10), (22, -14, -12), (26, 10, -8), (-8, 22, -14),
          (-20, 8, 9), (10, -6, 13), (12, 14, 6)],
}

EPSILON = 1e-4
FAR = 1000.0  # edge distance of a triangle side that is no edge of the solid


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def scale(p, factor):
    return (p[0] * factor, p[1] * factor, p[2] * factor)


def normalized(p):
    return scale(p, 1.0 / math.sqrt(dot(p, p)))


def centre(points):
    return tuple(sum(p[axis] for p in points) / len(points) for axis in range(3))


def cyclic(x, y, z):
    """The point and its two cyclic permutations."""
    return [(x, y, z), (y, z, x), (z, x, y)]


def signs(points):
    """Every sign combination of each point's coordinates, without duplicates."""
    result = []
    for p in points:
        for flips in itertools.product((1, -1), repeat=3):
            q = tuple(c * f for c, f in zip(p, flips))
            if q not in result:
                result.append(q)
    return result


def face_up(points):
    """Points turned so a face lies flat on top, its first corner toward +X (screen up)."""
    face = [points[i] for i in hull_faces(points)[0]]
    up = normalized(centre(face))
    axis = cross(up, (0.0, 0.0, 1.0))
    sin = math.sqrt(dot(axis, axis))
    cos = up[2]
    if sin > EPSILON:
        axis = scale(axis, 1.0 / sin)
        # Rodrigues: up onto +Z around up x Z.
        points = [tuple(p[i] * cos + cross(axis, p)[i] * sin + axis[i] * dot(axis, p) * (1.0 - cos) for i in range(3))
                  for p in points]
    first = max(points, key=lambda p: p[2])
    yaw = -math.atan2(first[1], first[0])
    return [(p[0] * math.cos(yaw) - p[1] * math.sin(yaw), p[0] * math.sin(yaw) + p[1] * math.cos(yaw), p[2])
            for p in points]


def diamond(radius):
    step = 2.0 * math.pi / DIAMOND_SIDES
    girdle = [(radius * math.cos(step * (i + 0.5)), radius * math.sin(step * (i + 0.5)), 0.0)
              for i in range(DIAMOND_SIDES)]
    table = [(DIAMOND_TABLE * radius * math.cos(step * i), DIAMOND_TABLE * radius * math.sin(step * i),
              DIAMOND_CROWN * radius) for i in range(DIAMOND_SIDES)]
    return girdle + table + [(0.0, 0.0, -DIAMOND_PAVILION * radius)]


def centred(points):
    """Points moved so their bounding box is centred on the origin."""
    middle = [(min(p[axis] for p in points) + max(p[axis] for p in points)) * 0.5 for axis in range(3)]
    return [sub(p, middle) for p in points]


def tier_points(tier):
    """Chip: octahedron, a corner up. Cut: dodecahedron, Prism: icosahedron, a face on top. Core: diamond, table up."""
    radius = RADIUS[tier]

    def on_sphere(points):
        return [scale(normalized(p), radius) for p in points]

    if tier == "Chip":
        return on_sphere(signs([(1, 0, 0), (0, 1, 0), (0, 0, 1)]))
    if tier == "Cut":
        return centred(face_up(on_sphere(signs([(1, 1, 1)] + cyclic(0, 1 / PHI, PHI)))))
    if tier == "Prism":
        return centred(face_up(on_sphere(signs(cyclic(0, 1, PHI)))))
    return centred(diamond(radius))


def hull_faces(points):
    """Faces of the convex hull: index lists CCW seen from outside, coplanar points merged into one face."""
    faces = {}
    for i, j, k in itertools.combinations(range(len(points)), 3):
        normal = cross(sub(points[j], points[i]), sub(points[k], points[i]))
        length = math.sqrt(dot(normal, normal))
        if length < EPSILON:
            continue
        normal = scale(normal, 1.0 / length)
        heights = [dot(normal, sub(p, points[i])) for p in points]
        if all(h >= -EPSILON for h in heights):
            normal = scale(normal, -1.0)
            heights = [-h for h in heights]
        if all(h <= EPSILON for h in heights):
            faces[frozenset(n for n, h in enumerate(heights) if h > -EPSILON)] = normal
    return [ordered_face(points, sorted(face), normal) for face, normal in faces.items()]


def ordered_face(points, face, normal):
    """Face corners sorted CCW around their centre seen from outside."""
    middle = centre([points[i] for i in face])
    u = normalized(sub(points[face[0]], middle))
    v = cross(normal, u)
    return sorted(face, key=lambda i: math.atan2(dot(sub(points[i], middle), v), dot(sub(points[i], middle), u)))


def edge_distances(corners, real):
    """Per corner (A, B, C): distances to its real edges in side order AB, BC, CA, the missing ones FAR."""
    a, b, c = corners
    area2 = math.sqrt(dot(cross(sub(b, a), sub(c, a)), cross(sub(b, a), sub(c, a))))

    def height(p, q):
        return area2 / math.dist(p, q)

    heights = {0: height(b, c), 1: height(c, a), 2: height(a, b)}  # corner k -> its opposite side
    # Side index: 0 = AB, 1 = BC, 2 = CA. Corner A is off BC, B off CA, C off AB.
    off_side = {0: 1, 1: 2, 2: 0}
    assert sum(real) <= 2, "a triangle holds two real edges at most"
    result = []
    for corner in range(3):
        row = [0.0, 0.0, 0.0]
        row[off_side[corner]] = heights[corner]
        distances = [row[side] for side in range(3) if real[side]]
        result.append(distances + [FAR] * (2 - len(distances)))
    return result


def check_hull(name, faces):
    """The hull closed: every edge shared by two faces, once each way."""
    edges = {}
    for face in faces:
        for a, b in zip(face, face[1:] + face[:1]):
            edges[(a, b)] = edges.get((a, b), 0) + 1
    for (a, b), count in edges.items():
        if count != 1 or edges.get((b, a), 0) != 1:
            raise ValueError(f"{name}: hull is not closed at edge {a}-{b}")


def build(asset_name, fill, slots):
    """Create (or rewrite in place) and save the asset; fill(vertex, tri, smooth) adds its triangles, slot by slot."""
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()
    groups = {}
    for slot in slots:
        groups[slot] = desc.create_polygon_group()
        desc.set_polygon_group_material_slot_name(groups[slot], slot)

    def vertex(position):
        v = desc.create_vertex()
        desc.set_vertex_position(v, unreal.Vector(*position))
        return v, position

    def tri(va, vb, vc, real):
        """Triangle ABC; real says which of its sides AB, BC, CA are edges of the solid."""
        corners = (va, vb, vc)
        distances = edge_distances([c[1] for c in corners], real)
        instances = []
        for corner, (first, second) in zip(corners, distances):
            instance = desc.create_vertex_instance(corner[0])
            desc.set_vertex_instance_uv(instance, unreal.Vector2D(first, second), 0)
            instances.append(instance)

        # Wound CCW seen from outside; UE front faces are clockwise, so reverse here.
        desc.create_triangle(groups[SLOT], instances[::-1])

    def smooth(positions, triangles):
        """Halo: corners shared between triangles, so the normals are averaged into a round surface."""
        instances = [desc.create_vertex_instance(vertex(p)[0]) for p in positions]
        for a, b, c in triangles:
            desc.create_triangle(groups[HALO_SLOT], [instances[c], instances[b], instances[a]])

    fill(vertex, tri, smooth)

    return mesh_toolkit["save_static_mesh"](FOLDER, asset_name, desc,
                                            [unreal.StaticMaterial(material_slot_name=slot) for slot in slots])


def halo_sphere(radius):
    """Icosphere: positions and CCW triangles of the icosahedron split HALO_SUBDIVISIONS times, pushed onto radius."""
    positions = [normalized(p) for p in signs(cyclic(0, 1, PHI))]
    triangles = [tuple(face) for face in hull_faces(positions)]
    for _ in range(HALO_SUBDIVISIONS):
        middles = {}

        def middle(a, b):
            key = (min(a, b), max(a, b))
            if key not in middles:
                middles[key] = len(positions)
                positions.append(normalized(tuple(x + y for x, y in zip(positions[a], positions[b]))))
            return middles[key]

        split = []
        for a, b, c in triangles:
            ab, bc, ca = middle(a, b), middle(b, c), middle(c, a)
            split += [(a, ab, ca), (ab, b, bc), (ca, bc, c), (ab, bc, ca)]
        triangles = split
    return [scale(p, radius) for p in positions], triangles


def fill_solid(name, points, halo_radius=0.0):
    """Each face split around its centre, one triangle per face edge; plus the halo sphere when halo_radius is set."""
    faces = hull_faces(points)
    check_hull(name, faces)

    def fill(vertex, tri, smooth):
        for face in faces:
            center = vertex(centre([points[i] for i in face]))
            for p, q in zip(face, face[1:] + face[:1]):
                tri(vertex(points[p]), vertex(points[q]), center, (True, False, False))
        if halo_radius > 0.0:
            smooth(*halo_sphere(halo_radius))
    return fill


def main():
    result = {"meshes": {}}
    try:
        jobs = [(f"SM_Gem_{tier}", fill_solid(tier, tier_points(tier), HALO_SCALE * RADIUS[tier]), [SLOT, HALO_SLOT])
                for tier in RADIUS]
        jobs += [(f"SM_Shard_{name}", fill_solid(f"shard {name}", [scale(p, SHARD_SCALE) for p in points]), [SLOT])
                 for name, points in SHARDS.items()]
        for asset_name, fill, slots in jobs:
            asset = build(asset_name, fill, slots)
            result["meshes"][asset_name] = {
                "num_triangles": asset.get_num_triangles(0),
                "num_vertices": asset.get_num_vertices(0),
                "bounds": str(asset.get_bounds().box_extent),
                "slots": [str(m.material_slot_name) for m in asset.get_editor_property("static_materials")],
            }
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "gem_meshes_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
