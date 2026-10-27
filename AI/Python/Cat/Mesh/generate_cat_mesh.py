"""
Generates SM_Cat: the Tutorial Cat, face toward +X, centred on Y = 0, built from separate flat-shaded parts so a rig
can later weight each to its own bone.

Face = the HUD portrait (SGeoCatFace.cpp, 200x200 design box, y down) extruded: head (polygon + rounded rectangle),
two ears with inner shades, two tilted rounded eyes, nose, two mouth curves, two fangs under the grin (hidden by every
clip but the evil face). Rayman-style, nothing touches: the body is
a three-sided pyramid floating behind the head, apex pointing back (a triangle from above), four disc paws (two hands at
the head's sides, two feet beside the body), the tail a row of small squares curling sideways from the apex.

Slots = the Cat's colour roles (AI/Tutorial/Cat.md): `Base`, `Shade`, `Ink`, `Light`. Part names and bounds are written
to AI/Output/cat_gen.json for the rig.

Run via mcp-unreal execute_script (or Runtime/run_via_bridge.py). Re-runnable: rewrites the asset in place.
"""
import json
import math

import unreal

mesh_toolkit_path = unreal.Paths.project_dir() + "AI/Python/Mesh/static_mesh_asset.py"
mesh_toolkit = {}
exec(compile(open(mesh_toolkit_path).read(), mesh_toolkit_path, "exec"), mesh_toolkit)

ASSET_NAME = "SM_Cat"
FOLDER = "/Game/Art/Meshes/Cat"
PACKAGE = f"{FOLDER}/{ASSET_NAME}"
BASE, SHADE, INK, LIGHT = "Base", "Shade", "Ink", "Light"
SLOTS = (BASE, SHADE, INK, LIGHT)

SCALE = 0.5  # design unit -> mesh unit
HEAD_CENTER_Z = 69.0  # of the design point (100, 118)
HEAD_BACK_X = 0.0
HEAD_FRONT_X = 30.0
EAR_BACK_X = 8.0
EAR_FRONT_X = 22.0
FEATURE_DEPTH = 1.5  # inner ears, eyes, nose, mouth stand this far off the surface they sit on
FANG_GAP = 0.5  # fangs float this far over the mouth, so no mouth vertex falls in a fang's box when rigging
POLYGON_SETBACK = 0.2  # the head polygon's front sits behind the rounded rectangle's so the overlap does not z-fight
MOUTH_HALF_THICKNESS = 1.5  # design units
CURVE_SEGMENTS = 8
CORNER_RADIUS = 44.0
EYE_TILT_DEGREES = 9.0

BODY_GAP = 8.0  # between the head's back (X = 0) and the body's base
BODY_LENGTH = 70.0  # base to apex
BODY_BASE_RADIUS = 25.0  # circumradius of the triangular base
BODY_CENTER_Z = 62.0
PAW_RADIUS = 7.0
PAW_SIDES = 20
HAND_Y = 52.0
HAND_X = (10.0, 22.0)
FOOT_Y = 26.0
FOOT_X = (-62.0, -50.0)
FOOT_Z = 44.0

TAIL_SQUARES = 9
TAIL_FIRST_TURN_DEGREES = 0.0  # sideways turn of the first step, from straight back
TAIL_TURN_DEGREES = 8.0  # added per square
TAIL_SPACING = 13.0
TAIL_SIZE = (10.0, 5.0)  # side of the first and the last square

HEAD_POLYGON = ((38, 64), (162, 64), (178, 104), (168, 112), (180, 128), (166, 138), (170, 152), (100, 176), (30, 152),
                (34, 138), (20, 128), (32, 112), (22, 104))
EARS = (((44, 60), (56, 16), (96, 52)), ((156, 60), (144, 16), (104, 52)))
INNER_EARS = (((58, 26), (70, 56), (50, 54)), ((142, 26), (130, 56), (150, 54)))
NOSE = ((95, 128), (105, 128), (100, 134))
EYE_X = (70.0, 130.0)
EYE_CENTER_Y = 106.0
MOUTH_CURVES = (((86, 139), (93, 149), (100, 139)), ((100, 139), (107, 149), (114, 139)))
FANGS = (((88, 142), (94, 142), (91, 151)), ((106, 142), (112, 142), (109, 151)))

FACE = lambda u, v, w: (w, u, v)  # u sideways, v up, w forward: a cyclic permutation keeps the winding
FLAT = lambda u, v, w: (u, v, w)


def signed_area(points):
    return 0.5 * sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(points, points[1:] + points[:1]))


def counter_clockwise(points):
    return points if signed_area(points) > 0.0 else points[::-1]


def design_to_face(point):
    return ((point[0] - 100.0) * SCALE, HEAD_CENTER_Z + (118.0 - point[1]) * SCALE)


def face_outline(design_points):
    return counter_clockwise([design_to_face(p) for p in design_points])


def curve(start, control, end):
    """Points after `start` up to and including `end` of the quadratic curve."""
    points = []
    for step in range(1, CURVE_SEGMENTS + 1):
        t = step / CURVE_SEGMENTS
        points.append(((1 - t) ** 2 * start[0] + 2 * (1 - t) * t * control[0] + t * t * end[0],
                       (1 - t) ** 2 * start[1] + 2 * (1 - t) * t * control[1] + t * t * end[1]))
    return points


def rounded_rectangle(x0, y0, x1, y1, radius, steps):
    corners = ((x1 - radius, y1 - radius, 0.0), (x0 + radius, y1 - radius, 90.0),
               (x0 + radius, y0 + radius, 180.0), (x1 - radius, y0 + radius, 270.0))
    points = []
    for cx, cy, start in corners:
        for step in range(steps + 1):
            angle = math.radians(start + 90.0 * step / steps)
            points.append((cx + radius * math.cos(angle), cy + radius * math.sin(angle)))
    return points


def rotate(points, degrees, pivot):
    c, s = math.cos(math.radians(degrees)), math.sin(math.radians(degrees))
    return [(pivot[0] + (x - pivot[0]) * c - (y - pivot[1]) * s, pivot[1] + (x - pivot[0]) * s + (y - pivot[1]) * c)
            for x, y in points]


def eye_shape(x):
    """The portrait's eye: flat bottom, round top, concave lower edge."""
    left, right = (x - 9.0, 120.0), (x + 9.0, 120.0)
    points = [left, (x - 9.0, 101.0)]
    points += curve((x - 9.0, 101.0), (x - 9.0, 92.0), (x, 92.0))
    points += curve((x, 92.0), (x + 9.0, 92.0), (x + 9.0, 101.0))
    points.append(right)
    points += curve(right, (x, 102.6), left)
    return points[:-1]  # the last point closes onto the first


def stroke(points, half_thickness):
    """Outline of a polyline thickened to both sides."""
    left, right = [], []
    for i, (x, y) in enumerate(points):
        before, after = points[max(i - 1, 0)], points[min(i + 1, len(points) - 1)]
        length = math.hypot(after[0] - before[0], after[1] - before[1])
        nx, ny = -(after[1] - before[1]) / length, (after[0] - before[0]) / length
        left.append((x + nx * half_thickness, y + ny * half_thickness))
        right.append((x - nx * half_thickness, y - ny * half_thickness))
    return left + right[::-1]


def circle(radius, sides):
    return [(radius * math.cos(2.0 * math.pi * i / sides), radius * math.sin(2.0 * math.pi * i / sides))
            for i in range(sides)]


def rectangle(x0, y0, x1, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def triangulate(outline):
    """Ear clipping of a simple CCW polygon: index triples, each CCW."""
    def cross(a, b, c):
        return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])

    def inside(p, a, b, c):
        return cross(a, b, p) >= 0.0 and cross(b, c, p) >= 0.0 and cross(c, a, p) >= 0.0

    remaining = list(range(len(outline)))
    triangles = []
    while len(remaining) > 3:
        for i in range(len(remaining)):
            ia, ib, ic = remaining[i - 1], remaining[i], remaining[(i + 1) % len(remaining)]
            a, b, c = outline[ia], outline[ib], outline[ic]
            if cross(a, b, c) > 1e-9 and not any(inside(outline[j], a, b, c) for j in remaining
                                                  if j not in (ia, ib, ic)):
                triangles.append((ia, ib, ic))
                remaining.pop(i)
                break
        else:
            raise RuntimeError("triangulate: no ear, the outline is not a simple polygon")
    triangles.append(tuple(remaining))
    return triangles


def pyramid():
    """Three-sided pyramid, base on the plane X = -BODY_GAP (one vertex up), apex behind it, as triangles wound CCW
    seen from outside."""
    z, base_x = BODY_CENTER_Z, -BODY_GAP
    corners = [(base_x, BODY_BASE_RADIUS * math.cos(math.radians(90.0 + 120.0 * i)),
                z + BODY_BASE_RADIUS * math.sin(math.radians(90.0 + 120.0 * i))) for i in range(3)]
    apex = (base_x - BODY_LENGTH, 0.0, z)
    faces = [(corners[i], corners[(i + 1) % 3], apex) for i in range(3)] + [tuple(corners)]
    inside = (base_x - BODY_LENGTH / 4, 0.0, z)

    def outward(face):
        a, b, c = face
        ab, ac = [b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)]
        normal = (ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0])
        return sum(normal[i] * (a[i] - inside[i]) for i in range(3)) > 0.0

    return [face if outward(face) else face[::-1] for face in faces]


class Solid:
    def __init__(self, name, slot, triangles):
        self.name, self.slot, self.triangles = name, slot, triangles


class Part:
    def __init__(self, name, slot, outline, w0, w1, place):
        self.name, self.slot, self.outline, self.w0, self.w1, self.place = name, slot, outline, w0, w1, place


def build_parts():
    parts = []
    front = HEAD_FRONT_X

    head_rect = face_outline(rounded_rectangle(22.0, 60.0, 178.0, 156.0, CORNER_RADIUS, CURVE_SEGMENTS))
    parts.append(Part("HeadRound", BASE, head_rect, HEAD_BACK_X, front, FACE))
    parts.append(Part("HeadFur", BASE, face_outline(HEAD_POLYGON), HEAD_BACK_X, front - POLYGON_SETBACK, FACE))

    for side, name in enumerate(("EarLeft", "EarRight")):
        parts.append(Part(name, BASE, face_outline(EARS[side]), EAR_BACK_X, EAR_FRONT_X, FACE))
        parts.append(Part(name + "Inner", SHADE, face_outline(INNER_EARS[side]), EAR_FRONT_X,
                          EAR_FRONT_X + FEATURE_DEPTH, FACE))

    for side, name in enumerate(("EyeLeft", "EyeRight")):
        tilt = EYE_TILT_DEGREES if side == 0 else -EYE_TILT_DEGREES
        tilted = rotate(eye_shape(EYE_X[side]), tilt, (EYE_X[side], EYE_CENTER_Y))
        parts.append(Part(name, LIGHT, face_outline(tilted), front, front + FEATURE_DEPTH, FACE))

    for side, name in enumerate(("FangLeft", "FangRight")):
        parts.append(Part(name, LIGHT, face_outline(FANGS[side]), front + FEATURE_DEPTH + FANG_GAP,
                          front + 2.0 * FEATURE_DEPTH + FANG_GAP, FACE))

    parts.append(Part("Nose", LIGHT, face_outline(NOSE), front, front + FEATURE_DEPTH, FACE))
    for index, (start, control, end) in enumerate(MOUTH_CURVES):
        path = stroke([start] + curve(start, control, end), MOUTH_HALF_THICKNESS)
        parts.append(Part(f"Mouth{index}", INK, face_outline(path), front, front + FEATURE_DEPTH, FACE))

    parts.append(Solid("Body", BASE, pyramid()))
    paws = (("HandLeft", -HAND_Y, HAND_X, BODY_CENTER_Z), ("HandRight", HAND_Y, HAND_X, BODY_CENTER_Z),
            ("FootLeft", -FOOT_Y, FOOT_X, FOOT_Z), ("FootRight", FOOT_Y, FOOT_X, FOOT_Z))
    for name, y, depth, z in paws:
        disc = [(y + u, z + v) for u, v in circle(PAW_RADIUS, PAW_SIDES)]
        parts.append(Part(name, BASE, disc, depth[0], depth[1], FACE))

    center = (-BODY_GAP - BODY_LENGTH - TAIL_SPACING / 2, 0.0)  # x, y
    for index in range(TAIL_SQUARES):
        turn = math.radians(TAIL_FIRST_TURN_DEGREES + TAIL_TURN_DEGREES * index)
        half = (TAIL_SIZE[0] + (TAIL_SIZE[1] - TAIL_SIZE[0]) * index / (TAIL_SQUARES - 1)) / 2
        square = rotate(rectangle(-half, -half, half, half), math.degrees(turn), (0.0, 0.0))
        outline = [(x + center[0], y + center[1]) for x, y in square]
        parts.append(Part(f"Tail{index}", BASE if index % 2 == 0 else SHADE, outline, BODY_CENTER_Z - half,
                          BODY_CENTER_Z + half, FLAT))
        center = (center[0] - math.cos(turn) * TAIL_SPACING, center[1] + math.sin(turn) * TAIL_SPACING)
    return parts


def build(parts):
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

    def vertex_at(position):
        vertex = desc.create_vertex()
        desc.set_vertex_position(vertex, unreal.Vector(*position))
        return vertex

    def record(part, positions):
        bounds[part.name] = {"slot": part.slot, "min": [min(p[i] for p in positions) for i in range(3)],
                             "max": [max(p[i] for p in positions) for i in range(3)]}

    def add_solid(part, group):
        for corners in part.triangles:
            tri(group, *[vertex_at(corner) for corner in corners])

        record(part, [corner for corners in part.triangles for corner in corners])

    def add_extrusion(part, group):
        bottom, top, positions = [], [], []
        for u, v in part.outline:
            for ring, w in ((bottom, part.w0), (top, part.w1)):
                position = part.place(u, v, w)
                ring.append(vertex_at(position))
                positions.append(position)

        count = len(part.outline)
        for i in range(count):
            n = (i + 1) % count
            tri(group, bottom[i], bottom[n], top[n])
            tri(group, bottom[i], top[n], top[i])

        for a, b, c in triangulate(part.outline):
            tri(group, top[a], top[b], top[c])
            tri(group, bottom[a], bottom[c], bottom[b])

        record(part, positions)

    bounds = {}
    for part in parts:
        (add_solid if isinstance(part, Solid) else add_extrusion)(part, groups[part.slot])

    asset = mesh_toolkit["save_static_mesh"](FOLDER, ASSET_NAME, desc,
                                             [unreal.StaticMaterial(material_slot_name=slot) for slot in SLOTS])
    return asset, bounds


def main():
    result = {}
    try:
        asset, bounds = build(build_parts())
        result["ok"] = True
        result["path"] = PACKAGE
        result["num_triangles"] = asset.get_num_triangles(0)
        result["num_vertices"] = asset.get_num_vertices(0)
        result["bounds"] = str(asset.get_bounds().box_extent)
        result["slots"] = [str(m.material_slot_name) for m in asset.get_editor_property("static_materials")]
        result["parts"] = bounds
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "cat_gen.json", "w") as f:
        json.dump(result, f, indent=2)


main()
