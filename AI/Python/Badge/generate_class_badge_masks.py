"""Class badge line masks: the white lines each badge carries on both caps, drawn from the badge's own outlines.

Per class, in GamePictures/BadgeMasks/ (pixel frame of generate_class_badge_meshes.py, 341 x 341):
  <Class>_Generated.png  this script's drawing, overwritten on every run
  <Class>_Hand.png       optional drawing by hand, same size, never touched; composited over the generated one
  <Class>_Mask.png       the composite make_class_badge_materials.py imports (white, alpha = line)

Every movable part gets a ring just inside its outline plus a filled inner copy of its shape scaled about its centre.
The body gets its own contour and logo lines, with the movable parts' footprints cleared so no body line lands on a
part. Widths and marks are the tunables below; change one, rerun this, then make_class_badge_materials.py.

Runs on host Python (Pillow), no editor: `python AI/Python/Badge/generate_class_badge_masks.py`.
Report written to AI/Output/class_badge_masks.txt.
"""
import os
import sys
from unittest import mock

from PIL import Image, ImageChops, ImageDraw

# --- Tunables, in mask pixels ---
LINE_WIDTH = 17.0    # every contour: the Triangle's band, measured perpendicular to its edge on the old mask
THIN_WIDTH = 6.0     # only where two parallel lines run side by side
RING_OFFSET = 0.0    # gap between a part's outline and its ring
BODY_INSET = 12.0    # gap between a body's outline and its contour line
PART_CLEAR = 6.0     # body lines stay this far from a movable part's outline
SUPERSAMPLE = 4

# Share each movable part's inner shape keeps of the part's size, by part kind.
INNER_SCALE = {"hourglass": 0.4, "bar": 0.3, "diamond": 0.3}
# Ring width by part kind where LINE_WIDTH would leave the part no room for its inner shape (the 40 px wide needle, the 46 px long mandibles).
RING_WIDTH = {"diamond": 8.0, "bar": 9.0}

# Square body logo: circuit line inset from the body outline, and the hollow squares ending it.
SQ_CIRCUIT_INSET = 30.0
SQ_TERMINAL_SIZE = 16.0
SQ_TERMINAL_DROP = 40.0  # terminal centre below the block's top edge
SQ_CHAMFER = 14.0        # the circuit's corner cut at the bottom

# Circle body middle line: x of the axis, top and bottom (the bottom ring takes over).
CIRCLE_MIDDLE_TOP = 150.0
CIRCLE_MIDDLE_BOTTOM = 304.0

CLASSES = {"Circle": "SM_CircleBadge", "Square": "SM_SquareBadge", "Triangle": "SM_TriangleBadge"}

PROJECT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
OUT_FOLDER = os.path.join(PROJECT, "GamePictures", "BadgeMasks")
REPORT = os.path.join(PROJECT, "AI", "Output", "class_badge_masks.txt")
SIZE = 341


def load_mesh_script():
    """generate_class_badge_meshes.py as a dict. Outside the editor `unreal` is a stand-in: only its path helper is real."""
    if "unreal" not in sys.modules:
        stub = mock.MagicMock()
        stub.Paths.project_dir.return_value = PROJECT.replace("\\", "/") + "/"
        sys.modules["unreal"] = stub

    path = os.path.join(PROJECT, "AI", "Python", "Badge", "Mesh", "generate_class_badge_meshes.py")
    namespace = {"__name__": "class_badge_meshes"}
    with open(path) as handle:
        exec(compile(handle.read(), path, "exec"), namespace)
    return namespace


class Layer:
    """A white-on-black coverage layer drawn at SUPERSAMPLE x, in the mask's pixel coordinates."""

    def __init__(self):
        self.image = Image.new("L", (SIZE * SUPERSAMPLE,) * 2, 0)

    def _scaled(self, points):
        return [(x * SUPERSAMPLE, y * SUPERSAMPLE) for x, y in points]

    def _band(self, points, width, closed):
        """The path stroked `width` wide, as its own coverage."""
        band = Image.new("L", self.image.size, 0)
        draw = ImageDraw.Draw(band)
        scaled = self._scaled(points + ([points[0]] if closed else []))
        draw.line(scaled, fill=255, width=max(1, round(width * SUPERSAMPLE)), joint="curve")
        for x, y in scaled:  # round caps
            radius = width * SUPERSAMPLE * 0.5
            draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=255)
        return band

    def _fill(self, points):
        filled = Image.new("L", self.image.size, 0)
        ImageDraw.Draw(filled).polygon(self._scaled(points), fill=255)
        return filled

    def add(self, coverage):
        self.image = ImageChops.lighter(self.image, coverage)

    def fill(self, points):
        self.add(self._fill(points))

    def polyline(self, points, width, closed=False):
        self.add(self._band(points, width, closed))

    def ring(self, outline, offset, width):
        """A stroke `width` wide whose outer edge sits `offset` inside the outline."""
        inside = self._fill(outline)
        outer = ImageChops.multiply(self._band(outline, 2.0 * (offset + width), True), inside)
        if offset > 0.0:
            outer = ImageChops.subtract(outer, self._band(outline, 2.0 * offset, True))
        self.add(outer)

    def erase(self, outline, margin):
        """Clears the outline's footprint grown by `margin`."""
        footprint = ImageChops.lighter(self._fill(outline), self._band(outline, 2.0 * margin, True))
        self.image = ImageChops.subtract(self.image, footprint)

    def merge(self, other):
        self.add(other.image)

    def result(self):
        return self.image.resize((SIZE, SIZE), Image.LANCZOS)


def centre(outline):
    xs = [p[0] for p in outline]
    ys = [p[1] for p in outline]
    return (min(xs) + max(xs)) * 0.5, (min(ys) + max(ys)) * 0.5


def scaled(outline, factor):
    cx, cy = centre(outline)
    return [(cx + (x - cx) * factor, cy + (y - cy) * factor) for x, y in outline]


def body_marks(badge, layer, body, parts):
    """The body's contour and logo lines, as polylines in mask pixels."""
    xs = [p[0] for p in body]
    ys = [p[1] for p in body]
    if badge == "Circle":
        layer.ring(body, BODY_INSET, LINE_WIDTH)
        cx = (min(xs) + max(xs)) * 0.5
        layer.polyline([(cx, CIRCLE_MIDDLE_TOP), (cx, CIRCLE_MIDDLE_BOTTOM)], THIN_WIDTH)
    elif badge == "Square":
        layer.ring(body, BODY_INSET, THIN_WIDTH)
        left, right = min(xs) + SQ_CIRCUIT_INSET, max(xs) - SQ_CIRCUIT_INSET
        bottom = max(ys) - SQ_CIRCUIT_INSET
        top = min(ys) + SQ_TERMINAL_DROP
        circuit = [(left, top), (left, bottom - SQ_CHAMFER), (left + SQ_CHAMFER, bottom),
                   (right - SQ_CHAMFER, bottom), (right, bottom - SQ_CHAMFER), (right, top)]
        layer.polyline(circuit, THIN_WIDTH)
        half = SQ_TERMINAL_SIZE * 0.5
        for x in (left, right):
            layer.ring([(x - half, top - half), (x + half, top - half), (x + half, top + half), (x - half, top + half)],
                       0.0, THIN_WIDTH)
    else:
        layer.ring(body, BODY_INSET, LINE_WIDTH)

    for part in parts:
        layer.erase(part.outline, PART_CLEAR)


def part_marks(layer, part):
    """A ring just inside the part's outline and a filled copy of its shape, scaled down about its centre."""
    layer.ring(part.outline, RING_OFFSET, RING_WIDTH.get(part.kind, LINE_WIDTH))
    layer.fill(scaled(part.outline, INNER_SCALE[part.kind]))


def draw_badge(mesh_script, badge, mesh_name):
    Part = mesh_script["PRISM"], mesh_script["WEDGE"]
    parts = mesh_script["badge_parts"]()[mesh_name]
    bodies = [part for part in parts if part.kind in Part]
    movable = [part for part in parts if part.kind not in Part]
    assert len(bodies) == 1, f"{mesh_name}: expected one body, found {len(bodies)}"

    layer = Layer()
    body_marks(badge, layer, bodies[0].outline, movable)
    for part in movable:
        part_marks(layer, part)
    return layer.result()


def write(badge, generated):
    os.makedirs(OUT_FOLDER, exist_ok=True)
    generated_path = os.path.join(OUT_FOLDER, f"{badge}_Generated.png")
    hand_path = os.path.join(OUT_FOLDER, f"{badge}_Hand.png")
    mask_path = os.path.join(OUT_FOLDER, f"{badge}_Mask.png")

    def coloured(coverage):
        image = Image.new("RGBA", coverage.size, (255, 255, 255, 0))
        image.putalpha(coverage)
        return image

    layer = coloured(generated)
    layer.save(generated_path)
    mask = layer
    note = "no hand layer"
    if os.path.exists(hand_path):
        hand = Image.open(hand_path).convert("RGBA")
        assert hand.size == layer.size, f"{hand_path}: {hand.size}, expected {layer.size}"
        mask = Image.alpha_composite(layer, hand)
        note = "hand layer on top"

    mask.save(mask_path)
    return f"{badge}: {mask_path} ({note})"


def main():
    log = []
    try:
        mesh_script = load_mesh_script()
        for badge, mesh_name in CLASSES.items():
            log.append(write(badge, draw_badge(mesh_script, badge, mesh_name)))
    except Exception:
        import traceback
        log.append(traceback.format_exc())

    os.makedirs(os.path.dirname(REPORT), exist_ok=True)
    with open(REPORT, "w") as handle:
        handle.write("\n".join(log))
    print("\n".join(log))


main()
