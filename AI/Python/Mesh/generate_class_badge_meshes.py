"""Generates the three class badge Static Meshes — SM_CircleBadge, SM_SquareBadge, SM_TriangleBadge.

Each is the class logo's silhouette traced from the source art and extruded straight up: the same outline
on three Z layers, vertical sides, a flat top. The pale inner lines of the logos are not geometry — they
belong to the material.

A badge is a list of parts that never touch, so a part is free to be driven on its own bone once the mesh
is rigged. The circle badge is two: the big disc carries a bite out of its top, and the small disc floats
free inside that bite, separated by the ring of black the art draws between them. The triangle badge is
two the same way — the arrowhead with its point cut off, and the needle standing free in the opening as
that point. The square badge is three: the block, and the two mandibles lifted off its top edge.

Most parts are PRISM, extruded to their badge's BODY_HEIGHT. The triangle's body is a WEDGE instead: that
height at its base, thinning toward the point to WEDGE_TIP of it at the apex, so the point still reads while
it rolls.
Three kinds turn in place rather than read as a flat sliver: the needle is a DIAMOND, a waist ring with
an apex above and below; each mandible is a BAR; and the circle's small disc is an HOURGLASS, pinched at
mid height so that turning it shows something a cylinder never would. All three are as deep as their own
narrow side is wide, so they turn without the silhouette swelling.

Outlines are authored in the source images' pixel space (341x341, Y down) so the numbers stay checkable
against the art, then mapped to world: image up -> +X (forward), image right -> +Y. A badge is centred on
the bounding box of all its parts together, or for one in CENTROID_ORIGIN on its footprint's centroid, so it
turns about where its weight sits. It is scaled so its longest side is BOX, matching the 100-unit bounds of
the SKM_Cube/Cone/Cylinder meshes it replaces — so all three keep one hitbox.

Caps are ear-clipped: the square and the triangle are concave, so a convex fan would fill their notches.

Run via mcp-unreal execute_script. Summary written to Saved/class_badge_gen.json.
"""
import collections
import json
import math

import unreal

FOLDER = "/Game/Characters/Meshes/Class"
SLOT_NAME = "Color"

TOL = 1e-6  # area below which an ear-clip corner counts as flat
BOX = 100.0  # longest XY side, matching the meshes these replace
BODY_HEIGHT = {"SM_CircleBadge": 75.0, "SM_SquareBadge": 85.0, "SM_TriangleBadge": 83.2}  # a WEDGE's at its base
WEDGE_TIP = 0.0  # share of its base height a WEDGE keeps at the badge's front
CENTROID_ORIGIN = ("SM_TriangleBadge",)  # centred on its footprint's centroid rather than its box

# --- Circle badge: a big disc bitten out around a small one floating free, in image pixels. ---
CIRCLE_BIG = (169.5, 200.0, 129.0)  # cx, cy, radius
CIRCLE_SMALL = (169.5, 75.0, 47.5)
CIRCLE_GAP = 13.5  # the black ring the art draws between the two; the bite is the small disc plus this
CIRCLE_WAIST = 0.55  # how far the small disc is pinched in at mid height
CIRCLE_BIG_SEGS = 20
CIRCLE_BITE_SEGS = 10
CIRCLE_SMALL_SEGS = 16

# --- Square badge: a chamfered block with two mandibles off its top edge, and a keyhole cut between. ---
SQ_CX = 168.0
SQ_HALF = 119.0        # body half-width
SQ_SHELF_HALF = 73.5   # raised top shelf half-width
SQ_TOP = 83.0          # top of the shelf
SQ_SHOULDER = 106.0    # where the shelf steps out to full width
SQ_BOTTOM = 292.0
SQ_CHAMFER = 18.0
SQ_HOLE = (168.0, 197.5, 36.5)  # cx, cy, radius
SQ_SLOT_HALF = 9.5
SQ_HOLE_SEGS = 14
SQ_MANDIBLE_GAP = 10.0    # black between a mandible and the block; 4.2 world units, as on the other two
SQ_MANDIBLE_EXTRA = 23.0  # reach past where the art ends them, so they read as jaws once they move

# --- Triangle badge: an arrowhead with a chevron bitten out of its base. ---
TRI_OUTLINE = [
    (168.0, 16.0),    # apex — the needle's own point, so the body stops short of it
    (292.0, 314.0),   # base, right
    (185.0, 301.0),   # chevron shoulder, right
    (168.0, 277.0),   # chevron tip
    (151.0, 301.0),   # chevron shoulder, left
    (44.0, 314.0),    # base, left
]

# --- Triangle needle: the pale diamond at the tip, a solid of its own standing in the body's opening. ---
NEEDLE_CX = 168.0      # on the body's axis, not the art's 170.5, since the body is symmetrised
NEEDLE_HALF = 15.0     # half-width at the waist, measured off the art's pale diamond
NEEDLE_GAP = 12.5      # black between needle and body; 4.47 world units, matching the circle's CIRCLE_GAP

PRISM, WEDGE, DIAMOND, BAR, HOURGLASS = "prism", "wedge", "diamond", "bar", "hourglass"
Part = collections.namedtuple("Part", "outline kind")


def arc(cx, cy, radius, start_deg, end_deg, segments, negative=False):
    """`segments` + 1 points from `start_deg` to `end_deg`, taking the sweep in the chosen direction."""
    span = (end_deg - start_deg) % 360.0 - (360.0 if negative else 0.0)
    return [(cx + radius * math.cos(math.radians(start_deg + span * i / segments)),
             cy + radius * math.sin(math.radians(start_deg + span * i / segments)))
            for i in range(segments + 1)]


def disc(cx, cy, radius, segments):
    """A whole circle as `segments` points."""
    return [(cx + radius * math.cos(2.0 * math.pi * i / segments),
             cy + radius * math.sin(2.0 * math.pi * i / segments))
            for i in range(segments)]


def circle_parts():
    """The big disc with a bite taken out of its top, and the small disc floating free inside that bite.

    The art keeps a black ring between the two rather than fusing them, so the bite is the small disc
    grown by CIRCLE_GAP. The pieces never touch, which is what lets each move on its own — and the small
    one is an HOURGLASS rather than a disc, so its turning is legible.
    """
    bx, by, big_r = CIRCLE_BIG
    sx, sy, small_r = CIRCLE_SMALL
    bite_r = small_r + CIRCLE_GAP
    d = by - sy  # centre distance; the small disc sits straight above the big one

    # Where the big circle and the bite cross, as an offset from the big centre along the line joining them.
    along = (d * d + big_r * big_r - bite_r * bite_r) / (2.0 * d)
    across = math.sqrt(big_r * big_r - along * along)

    # Image Y grows downward, so the crossings sit `along` above the big centre.
    big_right = math.degrees(math.atan2(-along, across))
    bite_right = math.degrees(math.atan2((by - along) - sy, across))

    # Round the big disc the long way, then back along the underside of the bite.
    body = (arc(bx, by, big_r, big_right, 180.0 - big_right, CIRCLE_BIG_SEGS)[:-1]
            + arc(sx, sy, bite_r, 180.0 - bite_right, bite_right, CIRCLE_BITE_SEGS, True)[:-1])
    return [Part(body, PRISM), Part(disc(sx, sy, small_r, CIRCLE_SMALL_SEGS), HOURGLASS)]


def square_parts():
    """The block, and the two mandibles that stood on its top edge, now free either side of the slot.

    The art draws the mandibles as one shelf continuous with the block, so cutting them loose costs them
    their root: the block's top edge drops back by SQ_MANDIBLE_GAP and each mandible is given
    SQ_MANDIBLE_EXTRA at the far end instead, reaching past where the shelf ever ended. The keyhole slot
    still runs out through the block's top edge, between the two.
    """
    hx, hy, hole_r = SQ_HOLE
    # The slot walls meet the hole where it is exactly as wide as they are.
    junction_y = hy - math.sqrt(hole_r * hole_r - SQ_SLOT_HALF * SQ_SLOT_HALF)
    junction_left = math.degrees(math.atan2(junction_y - hy, -SQ_SLOT_HALF))
    junction_right = math.degrees(math.atan2(junction_y - hy, SQ_SLOT_HALF))
    cut = SQ_SHOULDER + SQ_MANDIBLE_GAP  # the block stops short of where the shelf used to root

    block = ([(SQ_CX - SQ_HALF, SQ_BOTTOM - SQ_CHAMFER),
              (SQ_CX - SQ_HALF, cut),
              (hx - SQ_SLOT_HALF, cut),
              (hx - SQ_SLOT_HALF, junction_y)]
             # Round the bottom of the hole, the long way, so the slot stays open to the top edge.
             + arc(hx, hy, hole_r, junction_left, junction_right, SQ_HOLE_SEGS, True)[1:-1]
             + [(hx + SQ_SLOT_HALF, junction_y),
                (hx + SQ_SLOT_HALF, cut),
                (SQ_CX + SQ_HALF, cut),
                (SQ_CX + SQ_HALF, SQ_BOTTOM - SQ_CHAMFER),
                (SQ_CX + SQ_HALF - SQ_CHAMFER, SQ_BOTTOM),
                (SQ_CX - SQ_HALF + SQ_CHAMFER, SQ_BOTTOM)])

    tip = SQ_TOP - SQ_MANDIBLE_EXTRA
    mandibles = [Part([(inner, SQ_SHOULDER), (inner, tip), (outer, tip), (outer, SQ_SHOULDER)], BAR)
                 for inner, outer in ((hx - SQ_SLOT_HALF, SQ_CX - SQ_SHELF_HALF),
                                      (hx + SQ_SLOT_HALF, SQ_CX + SQ_SHELF_HALF))]
    return [Part(block, PRISM)] + mandibles


def needle_kite(grow):
    """The needle's four corners: the badge's own point, cut free as a rhombus and offset out by `grow`.

    Its tip is the apex itself and its waist corners sit on the body's two edges, so it is exactly the
    shape the arrowhead's point already had; the base mirrors the tip about the waist. Offsetting every
    edge outward by `grow` is, on a rhombus, a scale about its centre — so the black between needle and
    body keeps one width all the way round, as it does around the circle's small disc.
    """
    apex, base_right = TRI_OUTLINE[0], TRI_OUTLINE[1]
    half_height = NEEDLE_HALF * (base_right[1] - apex[1]) / (base_right[0] - apex[0])
    scale = 1.0 + grow * math.hypot(NEEDLE_HALF, half_height) / (NEEDLE_HALF * half_height)
    half_wide, half_long = NEEDLE_HALF * scale, half_height * scale
    cy = apex[1] + half_height
    return [(NEEDLE_CX, cy - half_long),
            (NEEDLE_CX + half_wide, cy),
            (NEEDLE_CX, cy + half_long),
            (NEEDLE_CX - half_wide, cy)]


def line_cross(a, b, c, d):
    """Where the line through a,b meets the line through c,d. They must not be parallel."""
    r = (b[0] - a[0], b[1] - a[1])
    s = (d[0] - c[0], d[1] - c[1])
    t = ((c[0] - a[0]) * s[1] - (c[1] - a[1]) * s[0]) / (r[0] * s[1] - r[1] * s[0])
    return (a[0] + t * r[0], a[1] + t * r[1])


def triangle_parts():
    """The arrowhead with its point opened out around the needle, and the needle standing in the opening.

    The needle is the point, so the body has to stop short of it: grown by NEEDLE_GAP it is wider than the
    arrowhead at its waist and severs the apex outright, leaving the body bounded by the grown rhombus's
    two trailing edges. The opening faces out, like the bite in the circle's big disc, and the body stays a
    simple polygon with no hole to triangulate around.
    """
    tip, right, base, left = needle_kite(NEEDLE_GAP)
    apex, base_right, chevron_right, chevron_tip, chevron_left, base_left = TRI_OUTLINE

    body = [line_cross(apex, base_left, left, base),
            base_left, chevron_left, chevron_tip, chevron_right, base_right,
            line_cross(apex, base_right, right, base), base]
    return [Part(body, WEDGE), Part(needle_kite(0.0), DIAMOND)]


def centroid(outline):
    """An outline's area centroid."""
    area = signed_area(outline)
    edges = [(a, b, a[0] * b[1] - b[0] * a[1]) for a, b in zip(outline, outline[1:] + outline[:1])]
    return (sum((a[0] + b[0]) * c for a, b, c in edges) / (6.0 * area),
            sum((a[1] + b[1]) * c for a, b, c in edges) / (6.0 * area))


def frame(name):
    """The badge's centre in image pixels and its world units per pixel. make_class_badge_materials.py maps by it too."""
    parts = badge_parts()[name]
    xs = [p[0] for part in parts for p in part.outline]
    ys = [p[1] for part in parts for p in part.outline]
    scale = BOX / max(max(xs) - min(xs), max(ys) - min(ys))
    if name in CENTROID_ORIGIN:
        weights = [(abs(signed_area(part.outline)), centroid(part.outline)) for part in parts]
        total = sum(weight for weight, _ in weights)
        return (sum(weight * c[0] for weight, c in weights) / total,
                sum(weight * c[1] for weight, c in weights) / total, scale)
    return (min(xs) + max(xs)) * 0.5, (min(ys) + max(ys)) * 0.5, scale


def to_world(name):
    """One badge's parts in world XY. One centre and scale for the whole badge, so its parts keep their offsets."""
    cx, cy, scale = frame(name)
    return [Part([((cy - y) * scale, (x - cx) * scale) for x, y in part.outline], part.kind)
            for part in badge_parts()[name]]


def signed_area(outline):
    return 0.5 * sum(a[0] * b[1] - b[0] * a[1]
                     for a, b in zip(outline, outline[1:] + outline[:1]))


def ear_clip(outline):
    """Triangulate a simple CCW polygon -> index triples. O(n^2), and n is a few dozen here."""
    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    def inside(p, a, b, c):
        # Strictly inside: a vertex sitting on an edge of a near-flat ear must not block it.
        return (cross(a, b, p) > TOL) and (cross(b, c, p) > TOL) and (cross(c, a, p) > TOL)

    remaining = list(range(len(outline)))
    triangles = []
    guard = len(remaining) * len(remaining)
    while len(remaining) > 2 and guard > 0:
        guard -= 1
        for k in range(len(remaining)):
            i, j, l = remaining[k - 1], remaining[k], remaining[(k + 1) % len(remaining)]
            a, b, c = outline[i], outline[j], outline[l]
            if cross(a, b, c) <= TOL:
                continue  # reflex or flat corner, never an ear

            if any(inside(outline[m], a, b, c) for m in remaining if m not in (i, j, l)):
                continue

            triangles.append((i, j, l))
            remaining.remove(j)
            break
        else:
            break  # no ear found: the outline is not simple
    return triangles


def build(name, parts):
    static_mesh = unreal.StaticMesh()
    desc = static_mesh.create_static_mesh_description()
    group = desc.create_polygon_group()
    desc.set_polygon_group_material_slot_name(group, SLOT_NAME)

    def tri(va, vb, vc):
        # Outlines are wound CCW seen from +Z; UE front faces are clockwise, so reverse here.
        desc.create_triangle(group, [desc.create_vertex_instance(vc),
                                     desc.create_vertex_instance(vb),
                                     desc.create_vertex_instance(va)])

    def vert(x, y, z):
        v = desc.create_vertex()
        desc.set_vertex_position(v, unreal.Vector(x, y, z))
        return v

    def narrow_side(outline):
        """Half the outline's extent across its narrower axis, and the midpoint of that span.

        The midpoint is the waist, not the box centre — on a kite the centre would lean the solid toward
        its long tip.
        """
        xs = [p[0] for p in outline]
        ys = [p[1] for p in outline]
        narrow = ys if (max(ys) - min(ys)) < (max(xs) - min(xs)) else xs
        low, high = outline[narrow.index(min(narrow))], outline[narrow.index(max(narrow))]
        return (0.5 * (max(narrow) - min(narrow)),
                ((low[0] + high[0]) * 0.5, (low[1] + high[1]) * 0.5))

    def diamond(outline):
        """The outline as a waist ring at z=0 with an apex above and below — 6 vertices, 8 triangles.

        The apexes stand as far off the plane as the outline is wide across its narrow axis, so that
        section is a square diamond.
        """
        half, (cx, cy) = narrow_side(outline)
        ring = [vert(x, y, 0.0) for x, y in outline]
        top, bottom = vert(cx, cy, half), vert(cx, cy, -half)
        for i in range(len(ring)):
            n = (i + 1) % len(ring)
            tri(ring[i], ring[n], top)
            tri(ring[n], ring[i], bottom)

    xs = [p[0] for part in parts for p in part.outline]
    body_half = 0.5 * BODY_HEIGHT[name]
    back, front = min(xs), max(xs)

    def wedge(x):
        """The share of its full height a WEDGE keeps at `x`: all of it at the badge's back, WEDGE_TIP at its front."""
        return 1.0 + (WEDGE_TIP - 1.0) * (x - back) / (front - back)

    def extrude(outline, profile, height):
        """One part: its own closed solid, the outline laid down once per (z, scale) layer of `profile`.

        Only the ends carry a cap, so every profile keeps its outline at full size there; a middle layer
        is free to pinch in. `height(x)` scales each layer's z where it stands, linear in x so a cap stays
        flat. Nothing is shared with any other part.
        """
        cx, cy = narrow_side(outline)[1]
        layers = []
        for z, s in profile:
            points = [(cx + (x - cx) * s, cy + (y - cy) * s) for x, y in outline]
            layers.append([vert(x, y, z * height(x)) for x, y in points])

        count = len(outline)
        for low, high in zip(layers, layers[1:]):
            for i in range(count):
                n = (i + 1) % count
                tri(low[i], low[n], high[n])
                tri(low[i], high[n], high[i])

        caps = ear_clip(outline)
        # A simple polygon always clips to n-2 ears; anything less means a hole in the cap.
        assert len(caps) == count - 2, "{}: {} ears for {} points".format(name, len(caps), count)
        for i, j, k in caps:
            tri(layers[-1][i], layers[-1][j], layers[-1][k])
            tri(layers[0][k], layers[0][j], layers[0][i])

    for part in parts:
        if part.kind == DIAMOND:
            diamond(part.outline)
        else:
            # A body takes the badge's full depth; a part that has to turn is as deep as it is narrow.
            half = body_half if part.kind in (PRISM, WEDGE) else narrow_side(part.outline)[0]
            waist = CIRCLE_WAIST if part.kind == HOURGLASS else 1.0
            height = wedge if part.kind == WEDGE else (lambda x: 1.0)
            extrude(part.outline, ((-half, 1.0), (0.0, waist), (half, 1.0)), height)

    package = "{}/{}".format(FOLDER, name)
    if unreal.EditorAssetLibrary.does_asset_exist(package):
        asset = unreal.load_asset(package)  # rewritten in place: a deleted loaded package stays unloadable
    else:
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, FOLDER, unreal.StaticMesh, None)

    asset.build_from_static_mesh_descriptions([desc], False, True)
    asset.set_editor_property("static_materials",
                              [unreal.StaticMaterial(material_slot_name=SLOT_NAME)])
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def badge_parts():
    """Every badge as its list of parts, in image pixels."""
    return {
        "SM_CircleBadge": circle_parts(),
        "SM_SquareBadge": square_parts(),
        "SM_TriangleBadge": triangle_parts(),
    }


def main():
    result = {}
    try:
        for name in badge_parts():
            parts = to_world(name)
            for part in parts:
                if signed_area(part.outline) < 0.0:
                    part.outline.reverse()

            asset = build(name, parts)
            result[name] = {
                "parts": ["{} {}".format(part.kind, len(part.outline)) for part in parts],
                "num_triangles": asset.get_num_triangles(0),
                "num_vertices": asset.get_num_vertices(0),
                "extent": str(asset.get_bounds().box_extent),
                "origin": str(asset.get_bounds().origin),
            }
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()

    with open(unreal.Paths.project_saved_dir() + "class_badge_gen.json", "w") as f:
        json.dump(result, f, indent=2)


if __name__ == "__main__":
    main()
