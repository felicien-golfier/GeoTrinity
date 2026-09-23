"""Glow looks for the floor lattice, one material layer each: MI_BackgroundLattice swaps or stacks them.

Needs make_generic_material_functions.py and make_background_lattice_material.py;
make_background_look_instances.py then gives each look the instance arenas cycle through. Run outside PIE.
"""
import unreal

FOLDER = "/Game/Art/VFX/Background"
FUNCTIONS = f"{FOLDER}/Functions"
LAYERS = f"{FOLDER}/Layers"
GENERIC = "/Game/Art/VFX/Generic/Materials/Functions"
CATEGORY = "GeoTrinity|Background"
SLOT_COUNT = 8
SLOT_FORMAT = "PulseSource_{:02d}"
LN_2 = 0.6931471805599453
TWO_PI = 6.283185307179586
# MF_SierpinskiMask repeats every 32 rows.
SIERPINSKI_PERIOD = 32.0
# Custom primitive data slots 1 and 2: the arena centre AGeoArena writes on its floors.
ARENA_CENTER_DATA_INDEX = 1

SOFT_RING_WIDTH = 160.0
SOFT_RING_CORE_WIDTH = 50.0
SOFT_RING_CORE_COLOR = unreal.LinearColor(0.6, 0.2, 0.5, 1.0)
SOFT_RING_EDGE_COLOR = unreal.LinearColor(0.1, 0.03, 0.3, 1.0)
SOFT_RING_BRIGHTNESS = 1.5

SHOCK_WAKE_LENGTH = 450.0
SHOCK_FRONT_WIDTH = 60.0
SHOCK_CORE_COLOR = unreal.LinearColor(0.85, 0.55, 1.0, 1.0)
SHOCK_EDGE_COLOR = unreal.LinearColor(0.12, 0.03, 0.25, 1.0)
SHOCK_BRIGHTNESS = 1.2

POLYGON_SIDES = 3.0
POLYGON_TWIST = 0.02
POLYGON_RING_WIDTH = 140.0
POLYGON_CORE_WIDTH = 40.0
POLYGON_CORE_COLOR = unreal.LinearColor(0.35, 0.3, 1.0, 1.0)
POLYGON_EDGE_COLOR = unreal.LinearColor(0.06, 0.04, 0.3, 1.0)
POLYGON_BRIGHTNESS = 1.2

HALO_CELL_SIZE = 200.0
HALO_RADIUS = 600.0
HALO_CORE_RADIUS = 250.0
HALO_BEAT = 0.5
HALO_CORE_COLOR = unreal.LinearColor(0.7, 0.25, 0.6, 1.0)
HALO_EDGE_COLOR = unreal.LinearColor(0.12, 0.04, 0.3, 1.0)
HALO_BRIGHTNESS = 1.0

SPIRAL_ARMS = 3.0
SPIRAL_PITCH = 900.0
SPIRAL_TURN_SPEED = 0.03
SPIRAL_WIDTH = 160.0
SPIRAL_CORE_WIDTH = 40.0
SPIRAL_REACH = 4500.0
SPIRAL_CORE_COLOR = unreal.LinearColor(0.5, 0.2, 0.9, 1.0)
SPIRAL_EDGE_COLOR = unreal.LinearColor(0.1, 0.03, 0.25, 1.0)
SPIRAL_BRIGHTNESS = 1.0

WHIRL_SIDES = 3.0
WHIRL_ZOOM_SPEED = 0.2
WHIRL_TWIST = 0.05
WHIRL_TURN_SPEED = 0.01
WHIRL_WIDTH = 120.0
WHIRL_CORE_WIDTH = 40.0
WHIRL_REACH = 4000.0
WHIRL_CORE_COLOR = unreal.LinearColor(0.9, 0.3, 0.8, 1.0)
WHIRL_EDGE_COLOR = unreal.LinearColor(0.15, 0.04, 0.35, 1.0)
WHIRL_BRIGHTNESS = 1.0

SIERPINSKI_CELL_SIZE = 200.0
SIERPINSKI_SCAN_SPEED = 4.0
SIERPINSKI_SCAN_WIDTH = 3.0
SIERPINSKI_CORE_WIDTH = 1.0
SIERPINSKI_REST = 0.15
SIERPINSKI_CORE_COLOR = unreal.LinearColor(0.8, 0.35, 1.0, 1.0)
SIERPINSKI_EDGE_COLOR = unreal.LinearColor(0.12, 0.04, 0.3, 1.0)
SIERPINSKI_BRIGHTNESS = 1.0

RADAR_BEAMS = 3.0
RADAR_TURN_SPEED = 0.1
RADAR_TRAIL = 0.15
RADAR_CORE_WIDTH = 60.0
RADAR_REACH = 4500.0
RADAR_CORE_COLOR = unreal.LinearColor(0.6, 0.35, 1.0, 1.0)
RADAR_EDGE_COLOR = unreal.LinearColor(0.08, 0.03, 0.25, 1.0)
RADAR_BRIGHTNESS = 1.0

TWINKLE_CELL_SIZE = 200.0
TWINKLE_RATE = 0.1
TWINKLE_FADE = 1.2
TWINKLE_CORE_COLOR = unreal.LinearColor(0.9, 0.4, 0.9, 1.0)
TWINKLE_EDGE_COLOR = unreal.LinearColor(0.15, 0.05, 0.35, 1.0)
TWINKLE_BRIGHTNESS = 1.0

FIREFLY_CELL_SIZE = 200.0
FIREFLY_SPREAD = 1500.0
FIREFLY_SPEED = 1.0
FIREFLY_RADIUS = 500.0
FIREFLY_CORE_RADIUS = 200.0
FIREFLY_CORE_COLOR = unreal.LinearColor(0.55, 0.3, 1.0, 1.0)
FIREFLY_EDGE_COLOR = unreal.LinearColor(0.08, 0.04, 0.3, 1.0)
FIREFLY_BRIGHTNESS = 1.0
# ((swings per second along X, along Y), (start of each swing, in turns)): unrelated, so no two lights move together.
FIREFLY_PATHS = (((0.050, 0.073), (0.0, 0.25)), ((0.061, 0.043), (0.4, 0.1)), ((0.037, 0.069), (0.7, 0.6)),
                 ((0.071, 0.057), (0.2, 0.85)), ((0.047, 0.031), (0.55, 0.35)))

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
open_function = toolkit["open_function"]
load = toolkit["load"]
scalar = unreal.MaterialExpressionScalarParameter
vector = unreal.MaterialExpressionVectorParameter

circle_distance = load(f"{GENERIC}/MF_CircleDistance")
polygon_distance = load(f"{GENERIC}/MF_PolygonDistance")
polar_coordinates = load(f"{GENERIC}/MF_PolarCoordinates")
repeat_distance = load(f"{GENERIC}/MF_RepeatDistance")
triangle_cell = load(f"{GENERIC}/MF_TriangleCell")
stroke_smooth = load(f"{GENERIC}/MF_Stroke_Smooth")
two_tone_stroke = load(f"{GENERIC}/MF_TwoToneStroke")
two_tone_color = load(f"{GENERIC}/MF_TwoToneColor")
sierpinski_mask = load(f"{GENERIC}/MF_SierpinskiMask")
random_from_position = load(f"{GENERIC}/MF_RandomFromPosition")
lissajous_point = load(f"{GENERIC}/MF_LissajousPoint")
collection = load(f"{FOLDER}/MPC_BackgroundPulse")
slot_names = [SLOT_FORMAT.format(index) for index in range(SLOT_COUNT)]


def unpack(graph, source, x, y):
    """A PulseSource slot's origin, radius and intensity."""
    return graph.mask(source, "rg", x, y), graph.mask(source, "b", x, y + 80), graph.mask(source, "a", x, y + 160)


def source_input(graph, x, y):
    return graph.input("PulseSource", "Vector4", 1, "(OriginX, OriginY, Radius, Intensity), in cm. An all-zero slot "
                       "draws nothing.", x, y)


def glow_outputs(graph, glow, core, glow_description, core_description, x, y):
    """The Glow and Core every MF_Wave_* gives, and the function finished."""
    graph.output("Glow", 0, glow_description, glow, x, y)
    graph.output("Core", 1, core_description, core, x, y + 160)
    graph.finish()
    return graph.asset


def brightest(graph, calls, x, y):
    """The largest Glow and the largest Core over calls that each give both."""
    glow = (calls[0], "Glow")
    core = (calls[0], "Core")
    for index, call in enumerate(calls[1:], 1):
        row = y + index * 200
        glow = graph.op(unreal.MaterialExpressionMax, x, row, A=glow, B=(call, "Glow"))
        core = graph.op(unreal.MaterialExpressionMax, x + 160, row + 60, A=core, B=(call, "Core"))

    return glow, core


def fan_out(graph, slot_function, what, pins, x, y):
    """slot_function once per MPC_BackgroundPulse slot, keeping the brightest Glow and Core over all of them."""
    calls = []
    for index, name in enumerate(slot_names):
        row = y + index * 200
        slot = graph.collection_parameter(collection, name, x, row + 40)
        calls.append(graph.call(slot_function, x + 300, row, PulseSource=slot, **pins))

    glow, core = brightest(graph, calls, x + 640, y)
    return glow_outputs(graph, glow, core, f"Brightness of the brightest {what}, 0 to 1.",
                        f"The part of Glow in the core colour, over every {what}.", x + 980, y + (SLOT_COUNT - 1) * 200)


def open_look(name, description):
    return toolkit["open_layer"](LAYERS, name, description, unreal.MaterialFunctionMaterialLayer,
                                 unreal.MaterialFunctionMaterialLayerFactory())


def world_xy(graph, x, y):
    return graph.mask(graph.node(unreal.MaterialExpressionWorldPosition, x - 240, y), "rg", x, y)


def arena_xy(graph, x, y):
    """The centre of the arena the floor belongs to, read from the floor's custom primitive data."""
    center = graph.parameter(vector, "ArenaCenter", unreal.LinearColor(0.0, 0.0, 0.0, 0.0), "Shape", 9,
                             "Centre of the arena this floor belongs to, in cm. The arena writes it on each of its "
                             "Floors; nothing to set here.", x - 240, y)
    center.set_editor_property("use_custom_primitive_data", True)
    center.set_editor_property("primitive_data_index", ARENA_CENTER_DATA_INDEX)
    return graph.mask(center, "rg", x, y)


def timed(graph, speed, x, y):
    """Time times a speed parameter: a phase that grows at that many units per second."""
    return graph.op(unreal.MaterialExpressionMultiply, x, y, A=graph.node(unreal.MaterialExpressionTime, x - 200, y),
                    B=speed)


def doublings(graph, length, x, y):
    """log2 of a length in metres, floored at 1 cm so the centre stays finite."""
    floored = graph.node(unreal.MaterialExpressionMax, x, y, const_b=1.0)
    graph.connect(length, floored, "A")
    metres = graph.node(unreal.MaterialExpressionMultiply, x + 120, y, const_b=0.01)
    graph.connect(floored, metres, "A")
    return graph.op(unreal.MaterialExpressionLogarithm2, x + 240, y, metres)


def dark_centre(graph, radius, hole, reach, x, y):
    """1 between the hole and Reach, easing to 0 at the centre and at Reach."""
    outer = graph.call(stroke_smooth, x, y, Distance=radius, Width=reach)
    near_centre = graph.call(stroke_smooth, x, y + 160, Distance=radius, Width=hole)
    inner = graph.op(unreal.MaterialExpressionOneMinus, x + 240, y + 160, (near_centre, "Mask"))
    return graph.op(unreal.MaterialExpressionMultiply, x + 380, y + 80, A=(outer, "Mask"), B=inner)


def finish_look(graph, wave, core_color, edge_color, brightness, x, y):
    """Colours the wave's Glow and Core, which is the whole of the layer's output."""
    parameters = (("CoreColor", core_color, 0, "Colour of the middle of the glow."),
                  ("EdgeColor", edge_color, 1, "Colour of the rest of the glow."),
                  ("Brightness", brightness, 2, "Glow strength, multiplying both colours."))
    nodes = {}
    for name, default, priority, description in parameters:
        cls = scalar if name == "Brightness" else vector
        nodes[name] = graph.parameter(cls, name, default, "Colour", priority, description, x - 300,
                                      y + 120 + priority * 120)

    color = graph.call(two_tone_color, x, y, Glow=(wave, "Glow"), Core=(wave, "Core"), **nodes)
    light = graph.op(unreal.MaterialExpressionMakeMaterialAttributes, x + 260, y, EmissiveColor=(color, "Color"))
    graph.output("Attributes", 0, "This look's glow alone.", light, x + 480, y)
    graph.finish()


# --- MF_SoftRing: one slot's ring, soft on both sides, two-tone ---
graph = open_function(FUNCTIONS, "MF_SoftRing",
                      "One two-tone ring of the background wave from a PulseSource slot of MPC_BackgroundPulse: soft "
                      "on both sides, a core along its middle.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1100, -120)
source = source_input(graph, -1100, 60)
ring_width = graph.input("RingWidth", "Scalar", 2, "Width of the ring at half brightness, in cm.", -1100, 300)
core_width = graph.input("CoreWidth", "Scalar", 3, "Width of its core at half strength, in cm.", -1100, 420)
origin, radius, intensity = unpack(graph, source, -840, -20)
from_ring = graph.call(circle_distance, -600, -80, Position=position, Center=origin, Radius=radius)
ring = graph.call(two_tone_stroke, -300, 60, Distance=(from_ring, "Distance"), Width=ring_width,
                  CoreWidth=core_width, Intensity=intensity)
soft_ring = glow_outputs(graph, (ring, "Glow"), (ring, "Core"), "Brightness of the ring, 0 to 1.",
                         "The part of Glow in the core colour.", 0, 60)

# --- MF_ShockRing: one slot's ring as a hard front with a soft wake behind it ---
graph = open_function(FUNCTIONS, "MF_ShockRing",
                      "One shock ring of the background wave from a PulseSource slot of MPC_BackgroundPulse: a hard, "
                      "bright front with a soft wake trailing behind it, toward the origin.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1200, -120)
source = source_input(graph, -1200, 60)
wake_length = graph.input("WakeLength", "Scalar", 2, "How far behind the front the wake fades out, in cm.", -1200, 300)
front_width = graph.input("FrontWidth", "Scalar", 3, "How far behind the front the core colour fades out, in cm.",
                          -1200, 420)
origin, radius, intensity = unpack(graph, source, -940, -20)
offset = graph.op(unreal.MaterialExpressionSubtract, -720, -100, A=position, B=origin)
from_origin = graph.op(unreal.MaterialExpressionLength, -580, -100, offset)
behind = graph.op(unreal.MaterialExpressionSubtract, -440, 0, A=radius, B=from_origin)
# Step(Y, X) is 1 where X >= Y: nothing ahead of the front.
inside = graph.node(unreal.MaterialExpressionStep, -300, 120, const_y=0.0)
graph.connect(behind, inside, "X")
strength = graph.op(unreal.MaterialExpressionMultiply, -160, 180, A=intensity, B=inside)
wake = graph.call(two_tone_stroke, 0, 60, Distance=behind, Width=wake_length, CoreWidth=front_width,
                  Intensity=strength)
shock_ring = glow_outputs(graph, (wake, "Glow"), (wake, "Core"),
                          "Brightness of the wake, 0 to 1, full at the front.",
                          "The part of Glow in the core colour: the front.", 300, 60)

# --- MF_PolygonRing: one slot's ring as a regular polygon turning as it grows ---
graph = open_function(FUNCTIONS, "MF_PolygonRing",
                      "One two-tone polygon ring of the background wave from a PulseSource slot of "
                      "MPC_BackgroundPulse: its corners on the slot's radius, a flat edge along the lattice at birth, "
                      "turning as it grows.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1300, -120)
source = source_input(graph, -1300, 60)
sides = graph.input("Sides", "Scalar", 2, "Number of sides, 3 or more.", -1300, 300)
twist = graph.input("Twist", "Scalar", 3, "Turn per metre of growth, in turns, counter-clockwise.", -1300, 420)
ring_width = graph.input("RingWidth", "Scalar", 4, "Width of the ring at half brightness, in cm.", -1300, 540)
core_width = graph.input("CoreWidth", "Scalar", 5, "Width of its core at half strength, in cm.", -1300, 660)
origin, radius, intensity = unpack(graph, source, -1040, -20)
local = graph.op(unreal.MaterialExpressionSubtract, -800, -100, A=position, B=origin)
grown_turns = graph.op(unreal.MaterialExpressionMultiply, -800, 280, A=radius, B=twist)
twist_turns = graph.node(unreal.MaterialExpressionMultiply, -660, 280, const_b=0.01)
graph.connect(grown_turns, twist_turns, "A")
# A quarter turn puts a flat edge on +Y, along the lattice's edges.
rotation = graph.node(unreal.MaterialExpressionAdd, -520, 280, const_b=0.25)
graph.connect(twist_turns, rotation, "A")
from_outline = graph.call(polygon_distance, -360, -40, Position=local, Sides=sides, Radius=radius, Rotation=rotation)
ring = graph.call(two_tone_stroke, -60, 60, Distance=(from_outline, "Distance"), Width=ring_width,
                  CoreWidth=core_width, Intensity=intensity)
polygon_ring = glow_outputs(graph, (ring, "Glow"), (ring, "Core"), "Brightness of the ring, 0 to 1.",
                            "The part of Glow in the core colour.", 240, 60)

# --- MF_PulseHalo: one slot's origin lighting the cells around it ---
graph = open_function(FUNCTIONS, "MF_PulseHalo",
                      "The halo a PulseSource slot of MPC_BackgroundPulse casts around its origin, one brightness per "
                      "triangle so whole cells light up. It can beat with the rings the slot sends.", CATEGORY)
cell_center = graph.input("CellCenter", "Vector2", 0, "Centre of the lattice cell being lit, in cm.", -1100, -120)
source = source_input(graph, -1100, 60)
halo_radius = graph.input("Radius", "Scalar", 2, "How far the halo reaches, in cm. Half bright at half of it.",
                          -1100, 300)
core_radius = graph.input("CoreRadius", "Scalar", 3, "How far the core colour reaches, in cm.", -1100, 420)
beat = graph.input("Beat", "Scalar", 4, "0 holds the halo steady; 1 flashes it with each ring the slot starts and "
                   "fades it as the ring grows.", -1100, 540)
origin = graph.mask(source, "rg", -840, -20)
intensity = graph.mask(source, "a", -840, 140)
offset = graph.op(unreal.MaterialExpressionSubtract, -620, -100, A=cell_center, B=origin)
from_origin = graph.op(unreal.MaterialExpressionLength, -480, -100, offset)
# A live slot has an intensity above 0, an empty one exactly 0.
live = graph.op(unreal.MaterialExpressionCeil, -620, 160, intensity)
beat_level = graph.node(unreal.MaterialExpressionLinearInterpolate, -620, 300, const_a=1.0)
graph.connect(intensity, beat_level, "B")
graph.connect(beat, beat_level, "Alpha")
strength = graph.op(unreal.MaterialExpressionMultiply, -440, 220, A=live, B=beat_level)
halo = graph.call(two_tone_stroke, -260, 60, Distance=from_origin, Width=halo_radius, CoreWidth=core_radius,
                  Intensity=strength)
pulse_halo = glow_outputs(graph, (halo, "Glow"), (halo, "Core"), "Brightness of the halo on this cell, 0 to 1.",
                          "The part of Glow in the core colour.", 40, 60)

# --- MF_Wave_* over the slots ---
graph = open_function(FUNCTIONS, "MF_Wave_SoftRings",
                      "Soft two-tone rings over every MPC_BackgroundPulse slot. BP_GeoCam's BackgroundPulse component "
                      "moves and grows them.", CATEGORY)
pins = {"Position": graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1100, 0),
        "RingWidth": graph.input("RingWidth", "Scalar", 1, "Width of a ring at half brightness, in cm.", -1100, 200),
        "CoreWidth": graph.input("CoreWidth", "Scalar", 2, "Width of a ring's core at half strength, in cm.", -1100,
                                 400)}
soft_rings = fan_out(graph, soft_ring, "ring", pins, -800, 0)

graph = open_function(FUNCTIONS, "MF_Wave_ShockRings",
                      "Shock rings over every MPC_BackgroundPulse slot: hard fronts, soft wakes. BP_GeoCam's "
                      "BackgroundPulse component moves and grows them.", CATEGORY)
pins = {"Position": graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1100, 0),
        "WakeLength": graph.input("WakeLength", "Scalar", 1, "How far behind a front its wake fades out, in cm.",
                                  -1100, 200),
        "FrontWidth": graph.input("FrontWidth", "Scalar", 2, "How far behind a front its core colour fades out, in "
                                  "cm.", -1100, 400)}
shock_rings = fan_out(graph, shock_ring, "ring", pins, -800, 0)

graph = open_function(FUNCTIONS, "MF_Wave_PolygonRings",
                      "Two-tone polygon rings over every MPC_BackgroundPulse slot, turning as they grow. BP_GeoCam's "
                      "BackgroundPulse component moves and grows them.", CATEGORY)
pins = {"Position": graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1100, 0),
        "Sides": graph.input("Sides", "Scalar", 1, "Number of sides, 3 or more.", -1100, 200),
        "Twist": graph.input("Twist", "Scalar", 2, "Turn per metre of growth, in turns.", -1100, 400),
        "RingWidth": graph.input("RingWidth", "Scalar", 3, "Width of a ring at half brightness, in cm.", -1100, 600),
        "CoreWidth": graph.input("CoreWidth", "Scalar", 4, "Width of a ring's core at half strength, in cm.", -1100,
                                 800)}
polygon_rings = fan_out(graph, polygon_ring, "ring", pins, -800, 0)

graph = open_function(FUNCTIONS, "MF_Wave_Halos",
                      "Halos around every MPC_BackgroundPulse origin, lighting whole lattice cells: under the "
                      "characters when BP_GeoCam's BackgroundPulse component tracks actors, drifting when it "
                      "wanders.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1400, -200)
cell_size = graph.input("CellSize", "Scalar", 1, "Side length of the lattice's triangles, in cm.", -1400, -60)
cell = graph.call(triangle_cell, -1100, -160, Position=position, Size=cell_size)
pins = {"CellCenter": (cell, "CellCenter"),
        "Radius": graph.input("Radius", "Scalar", 2, "How far a halo reaches, in cm.", -1400, 200),
        "CoreRadius": graph.input("CoreRadius", "Scalar", 3, "How far a halo's core colour reaches, in cm.", -1400,
                                  400),
        "Beat": graph.input("Beat", "Scalar", 4, "0 holds the halos steady; 1 flashes each with the rings it sends.",
                            -1400, 600)}
halos = fan_out(graph, pulse_halo, "halo", pins, -800, 0)

# --- MF_Wave_Spiral: arms wound around a centre, turning ---
graph = open_function(FUNCTIONS, "MF_Wave_Spiral",
                      "Two-tone spiral arms wound around a centre, evenly spaced, dark at the centre and fading out "
                      "at Reach.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1500, -200)
center = graph.input("Center", "Vector2", 1, "Centre of the spiral, in cm.", -1500, -80)
arms = graph.input("Arms", "Scalar", 2, "Number of arms, a whole number.", -1500, 40)
pitch = graph.input("Pitch", "Scalar", 3, "Distance between neighbouring arms along a radius, in cm.", -1500, 160)
turn = graph.input("Turn", "Scalar", 4, "Counter-clockwise turn of the whole spiral, in turns.", -1500, 280)
width = graph.input("Width", "Scalar", 5, "Width of an arm at half brightness, in cm.", -1500, 400)
core_width = graph.input("CoreWidth", "Scalar", 6, "Width of an arm's core at half strength, in cm.", -1500, 520)
reach = graph.input("Reach", "Scalar", 7, "Distance from the centre where the spiral has faded out, in cm.", -1500,
                    640)
local = graph.op(unreal.MaterialExpressionSubtract, -1260, -140, A=position, B=center)
polar = graph.call(polar_coordinates, -1100, -140, Position=local)
unwound = graph.op(unreal.MaterialExpressionSubtract, -860, -60, A=(polar, "Angle"), B=turn)
arm_turns = graph.op(unreal.MaterialExpressionMultiply, -720, -20, A=unwound, B=arms)
wound = graph.op(unreal.MaterialExpressionMultiply, -580, 20, A=arm_turns, B=pitch)
along = graph.op(unreal.MaterialExpressionSubtract, -440, -100, A=(polar, "Radius"), B=wound)
to_arm = graph.call(repeat_distance, -300, -60, Value=along, Spacing=pitch)
# Every arm meets at the centre: dark within half a pitch of it.
hole = graph.node(unreal.MaterialExpressionMultiply, -1060, 520, const_b=0.5)
graph.connect(pitch, hole, "A")
strength = dark_centre(graph, (polar, "Radius"), hole, reach, -860, 400)
spiral_arm = graph.call(two_tone_stroke, -60, 60, Distance=(to_arm, "Distance"), Width=width, CoreWidth=core_width,
                        Intensity=strength)
spiral = glow_outputs(graph, (spiral_arm, "Glow"), (spiral_arm, "Core"), "Brightness of the arms, 0 to 1.",
                      "The part of Glow in the core colour.", 240, 60)

# --- MF_Wave_Whirl: nested polygons, each twice the last, growing forever and twisting ---
graph = open_function(FUNCTIONS, "MF_Wave_Whirl",
                      "Two-tone nested regular polygons around a centre, each twice the size of the one inside it and "
                      "turned by Twist from it. Raising Zoom grows every polygon into the next one's place, so the "
                      "zoom never ends.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1700, -200)
center = graph.input("Center", "Vector2", 1, "Centre of the whirl, in cm.", -1700, -80)
sides = graph.input("Sides", "Scalar", 2, "Number of sides, 3 or more.", -1700, 40)
zoom = graph.input("Zoom", "Scalar", 3, "Growth, in doublings: 1 puts every polygon where the next one was.", -1700,
                   160)
twist = graph.input("Twist", "Scalar", 4, "Turn between a polygon and the next one out, in turns.", -1700, 280)
turn = graph.input("Turn", "Scalar", 5, "Counter-clockwise turn of the whole whirl, in turns.", -1700, 400)
width = graph.input("Width", "Scalar", 6, "Width of a line at half brightness, in cm.", -1700, 520)
core_width = graph.input("CoreWidth", "Scalar", 7, "Width of a line's core at half strength, in cm.", -1700, 640)
reach = graph.input("Reach", "Scalar", 8, "Distance from the centre where the whirl has faded out, in cm.", -1700, 760)
local = graph.op(unreal.MaterialExpressionSubtract, -1460, -140, A=position, B=center)
polar = graph.call(polar_coordinates, -1300, -140, Position=local)
radius_doublings = doublings(graph, (polar, "Radius"), -1060, 100)
# Each polygon keeps its turn while it grows: the twist follows its doubling, not the point's.
grown = graph.op(unreal.MaterialExpressionSubtract, -700, 140, A=radius_doublings, B=zoom)
twisted = graph.op(unreal.MaterialExpressionMultiply, -560, 180, A=grown, B=twist)
rotation = graph.op(unreal.MaterialExpressionAdd, -420, 220, A=twisted, B=turn)
no_radius = graph.node(unreal.MaterialExpressionConstant, -420, 0, r=0.0)
# At radius 0 the distance to the outline is the point's own distance along the nearest edge normal.
along = graph.call(polygon_distance, -260, -60, Position=local, Sides=sides, Radius=no_radius, Rotation=rotation)
along_doublings = doublings(graph, (along, "Distance"), -40, -160)
polygon_level = graph.op(unreal.MaterialExpressionSubtract, 320, -120, A=along_doublings, B=zoom)
one = graph.node(unreal.MaterialExpressionConstant, 320, 0, r=1.0)
to_polygon = graph.call(repeat_distance, 480, -80, Value=polygon_level, Spacing=one)
# One doubling spans along * ln 2 cm at this distance.
level_span = graph.node(unreal.MaterialExpressionMultiply, 480, 80, const_b=LN_2)
graph.connect((along, "Distance"), level_span, "A")
to_polygon_cm = graph.op(unreal.MaterialExpressionMultiply, 720, -20, A=(to_polygon, "Distance"), B=level_span)
# The polygons crowd into a blur toward the centre: dark within four line widths of it.
hole = graph.node(unreal.MaterialExpressionMultiply, 160, 600, const_b=4.0)
graph.connect(width, hole, "A")
strength = dark_centre(graph, (polar, "Radius"), hole, reach, 320, 400)
whirl_line = graph.call(two_tone_stroke, 880, 60, Distance=to_polygon_cm, Width=width, CoreWidth=core_width,
                        Intensity=strength)
whirl = glow_outputs(graph, (whirl_line, "Glow"), (whirl_line, "Core"), "Brightness of the lines, 0 to 1.",
                     "The part of Glow in the core colour.", 1180, 60)

# --- MF_Wave_Sierpinski: lattice cells picked out as a Sierpinski triangle, a band printing it row by row ---
graph = open_function(FUNCTIONS, "MF_Wave_Sierpinski",
                      "Lattice cells lit as a Sierpinski triangle, 32 rows high and repeating across the floor, dim at "
                      "rest while a band climbs through the rows like a printer printing Pascal's triangle.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1400, -160)
cell_size = graph.input("CellSize", "Scalar", 1, "Side length of the lattice's triangles, in cm.", -1400, -40)
scan = graph.input("Scan", "Scalar", 2, "Row the band is on, wrapping every 32 rows.", -1400, 200)
scan_width = graph.input("ScanWidth", "Scalar", 3, "Width of the band at half brightness, in rows.", -1400, 320)
core_width = graph.input("CoreWidth", "Scalar", 4, "Width of the band's core at half strength, in rows.", -1400, 440)
rest = graph.input("Rest", "Scalar", 5, "Brightness of the triangle away from the band, 0 to 1.", -1400, 560)
cell = graph.call(triangle_cell, -1160, -100, Position=position, Size=cell_size)
rows = graph.mask((cell, "CellIndex"), "rg", -900, -160)
pointing_up = graph.mask((cell, "CellIndex"), "b", -900, -40)
gasket = graph.call(sierpinski_mask, -720, -200, Coordinates=rows)
# The triangle is built from the cells pointing down; every cell pointing up is a hole.
pointing_down = graph.op(unreal.MaterialExpressionOneMinus, -720, -40, pointing_up)
lit = graph.op(unreal.MaterialExpressionMultiply, -480, -120, A=(gasket, "Mask"), B=pointing_down)
row_axis = graph.node(unreal.MaterialExpressionConstant2Vector, -900, 100, r=1.0, g=1.0)
row = graph.op(unreal.MaterialExpressionDotProduct, -720, 100, A=rows, B=row_axis)
from_scan_row = graph.op(unreal.MaterialExpressionSubtract, -560, 180, A=row, B=scan)
period = graph.node(unreal.MaterialExpressionConstant, -560, 280, r=SIERPINSKI_PERIOD)
from_scan = graph.call(repeat_distance, -400, 200, Value=from_scan_row, Spacing=period)
full = graph.node(unreal.MaterialExpressionConstant, -400, 400, r=1.0)
band = graph.call(two_tone_stroke, -160, 280, Distance=(from_scan, "Distance"), Width=scan_width, CoreWidth=core_width,
                  Intensity=full)
resting = graph.op(unreal.MaterialExpressionMax, 100, 240, A=(band, "Glow"), B=rest)
glow = graph.op(unreal.MaterialExpressionMultiply, 260, 0, A=lit, B=resting)
core = graph.op(unreal.MaterialExpressionMultiply, 260, 160, A=lit, B=(band, "Core"))
sierpinski = glow_outputs(graph, glow, core, "Brightness of the triangle's cells, 0 to 1.",
                          "The part of Glow in the core colour: the band.", 420, 0)

# --- MF_Wave_Radar: beams sweeping around a centre, each trailing a fading glow ---
graph = open_function(FUNCTIONS, "MF_Wave_Radar",
                      "Beams sweeping around a centre like a radar, evenly spaced: a hard bright edge leading, a glow "
                      "fading behind each, dark at the centre and fading out at Reach.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1400, -200)
center = graph.input("Center", "Vector2", 1, "Centre of the sweep, in cm.", -1400, -80)
beams = graph.input("Beams", "Scalar", 2, "Number of beams, a whole number.", -1400, 40)
turn = graph.input("Turn", "Scalar", 3, "Counter-clockwise turn of the beams, in turns.", -1400, 160)
trail = graph.input("Trail", "Scalar", 4, "How far behind a beam its glow fades out, in turns.", -1400, 280)
core_width = graph.input("CoreWidth", "Scalar", 5, "How far behind a beam its core colour fades out, in cm along the "
                         "sweep.", -1400, 400)
reach = graph.input("Reach", "Scalar", 6, "Distance from the centre where the sweep has faded out, in cm.", -1400, 520)
local = graph.op(unreal.MaterialExpressionSubtract, -1160, -140, A=position, B=center)
polar = graph.call(polar_coordinates, -1000, -140, Position=local)
lead = graph.op(unreal.MaterialExpressionSubtract, -760, -20, A=turn, B=(polar, "Angle"))
in_gaps = graph.op(unreal.MaterialExpressionMultiply, -620, 20, A=lead, B=beams)
gap_since = graph.op(unreal.MaterialExpressionFrac, -480, 20, in_gaps)
# Turns since the last beam passed.
behind = graph.op(unreal.MaterialExpressionDivide, -360, 60, A=gap_since, B=beams)
trail_line = graph.call(stroke_smooth, -160, -40, Distance=behind, Width=trail)
arc_turns = graph.op(unreal.MaterialExpressionMultiply, -360, 200, A=behind, B=(polar, "Radius"))
arc = graph.node(unreal.MaterialExpressionMultiply, -220, 200, const_b=TWO_PI)
graph.connect(arc_turns, arc, "A")
edge_line = graph.call(stroke_smooth, -60, 160, Distance=arc, Width=core_width)
hole = graph.node(unreal.MaterialExpressionMultiply, -760, 480, const_b=4.0)
graph.connect(core_width, hole, "A")
strength = dark_centre(graph, (polar, "Radius"), hole, reach, -560, 400)
glow = graph.op(unreal.MaterialExpressionMultiply, 200, 0, A=(trail_line, "Mask"), B=strength)
core = graph.op(unreal.MaterialExpressionMultiply, 200, 160, A=(edge_line, "Mask"), B=strength)
radar = glow_outputs(graph, glow, core, "Brightness of the beams and their trails, 0 to 1.",
                     "The part of Glow in the core colour: the leading edges.", 360, 0)

# --- MF_Wave_Twinkle: every cell flashing on its own random clock ---
graph = open_function(FUNCTIONS, "MF_Wave_Twinkle",
                      "Lattice cells flashing at random, each on its own clock: full at once, then fading out.",
                      CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1300, -160)
cell_size = graph.input("CellSize", "Scalar", 1, "Side length of the lattice's triangles, in cm.", -1300, -40)
time = graph.input("Time", "Scalar", 2, "Seconds, usually the Time node.", -1300, 120)
rate = graph.input("Rate", "Scalar", 3, "Flashes per second of each cell.", -1300, 240)
fade = graph.input("Fade", "Scalar", 4, "Seconds a flash takes to fade out.", -1300, 360)
cell = graph.call(triangle_cell, -1060, -100, Position=position, Size=cell_size)
# In cells rather than cm, which keeps the random values spread.
in_cells = graph.op(unreal.MaterialExpressionDivide, -820, -100, A=(cell, "CellCenter"), B=cell_size)
clock_offset = graph.call(random_from_position, -680, -100, Position=in_cells)
# A rate of 0 would divide by zero.
safe_rate = graph.node(unreal.MaterialExpressionMax, -1060, 240, const_b=0.001)
graph.connect(rate, safe_rate, "A")
ticks = graph.op(unreal.MaterialExpressionMultiply, -820, 160, A=time, B=safe_rate)
phase = graph.op(unreal.MaterialExpressionAdd, -540, 60, A=ticks, B=(clock_offset, "Value"))
cycle = graph.op(unreal.MaterialExpressionFrac, -420, 60, phase)
since_flash = graph.op(unreal.MaterialExpressionDivide, -300, 100, A=cycle, B=safe_rate)
core_fade = graph.node(unreal.MaterialExpressionMultiply, -1060, 400, const_b=0.25)
graph.connect(fade, core_fade, "A")
full = graph.node(unreal.MaterialExpressionConstant, -300, 360, r=1.0)
flash = graph.call(two_tone_stroke, -120, 160, Distance=since_flash, Width=fade, CoreWidth=core_fade, Intensity=full)
twinkle = glow_outputs(graph, (flash, "Glow"), (flash, "Core"), "Brightness of the cell's flash, 0 to 1.",
                       "The part of Glow in the core colour: the start of a flash.", 160, 160)

# --- MF_Wave_Fireflies: lights wandering on Lissajous paths, each lighting the cells around it like a halo ---
graph = open_function(FUNCTIONS, "MF_Wave_Fireflies",
                      "Five lights wandering over the lattice around a centre, each on its own slow Lissajous path, "
                      "each lighting whole cells the way a pulse halo does.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1500, -200)
center = graph.input("Center", "Vector2", 1, "Middle of the lights' paths, in cm.", -1500, -80)
cell_size = graph.input("CellSize", "Scalar", 2, "Side length of the lattice's triangles, in cm.", -1500, 40)
spread = graph.input("Spread", "Scalar", 3, "Furthest a light goes from Center along each axis, in cm.", -1500, 160)
time = graph.input("Time", "Scalar", 4, "Seconds, or any clock: a faster clock moves them faster.", -1500, 280)
halo_radius = graph.input("Radius", "Scalar", 5, "How far a light reaches, in cm.", -1500, 400)
core_radius = graph.input("CoreRadius", "Scalar", 6, "How far a light's core colour reaches, in cm.", -1500, 520)
cell = graph.call(triangle_cell, -1260, -160, Position=position, Size=cell_size)
# A pulse slot with radius 0 and intensity 1, and no beat: a steady halo.
lit = graph.node(unreal.MaterialExpressionConstant2Vector, -1000, 640, r=0.0, g=1.0)
steady = graph.node(unreal.MaterialExpressionConstant, -760, 700, r=0.0)
halos_around = []
for index, ((frequency_x, frequency_y), (phase_x, phase_y)) in enumerate(FIREFLY_PATHS):
    row = index * 240
    frequency = graph.node(unreal.MaterialExpressionConstant2Vector, -1260, row + 80, r=frequency_x, g=frequency_y)
    phase = graph.node(unreal.MaterialExpressionConstant2Vector, -1260, row + 160, r=phase_x, g=phase_y)
    point = graph.call(lissajous_point, -1000, row, Center=center, Spread=spread, Time=time, Frequency=frequency,
                       Phase=phase)
    source = graph.op(unreal.MaterialExpressionAppendVector, -760, row + 40, A=(point, "Point"), B=lit)
    halos_around.append(graph.call(pulse_halo, -560, row, CellCenter=(cell, "CellCenter"), PulseSource=source,
                                   Radius=halo_radius, CoreRadius=core_radius, Beat=steady))

glow, core = brightest(graph, halos_around, -240, 0)
fireflies = glow_outputs(graph, glow, core, "Brightness of the brightest light on this cell, 0 to 1.",
                         "The part of Glow in the core colour.", 120, 800)

# --- ML_Wave_*: the looks, each a glow layer ---
graph = open_look("ML_Wave_SoftRings",
                  "Glow layer of M_BackgroundLattice: the pulse rings, soft on both sides, one colour along their "
                  "middle and another toward their edges.")
ring_width = graph.parameter(scalar, "RingWidth", SOFT_RING_WIDTH, "Shape", 0,
                             "Width of a ring at half brightness, in cm. Ring speed, reach and count live on "
                             "BP_GeoCam > BackgroundPulse.", -760, 100)
core_width = graph.parameter(scalar, "CoreWidth", SOFT_RING_CORE_WIDTH, "Shape", 1,
                             "Width of the core colour along a ring's middle, at half strength, in cm. Narrower than "
                             "RingWidth.", -760, 200)
wave = graph.call(soft_rings, -460, 0, Position=world_xy(graph, -760, 0), RingWidth=ring_width, CoreWidth=core_width)
finish_look(graph, wave, SOFT_RING_CORE_COLOR, SOFT_RING_EDGE_COLOR, SOFT_RING_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_ShockRings",
                  "Glow layer of M_BackgroundLattice: the pulse rings as shock fronts, a hard bright edge leading, "
                  "a soft wake of another colour trailing toward the origin.")
wake_length = graph.parameter(scalar, "WakeLength", SHOCK_WAKE_LENGTH, "Shape", 0,
                              "How far behind a front its wake fades out, in cm. Ring speed, reach and count live on "
                              "BP_GeoCam > BackgroundPulse.", -760, 100)
front_width = graph.parameter(scalar, "FrontWidth", SHOCK_FRONT_WIDTH, "Shape", 1,
                              "How far behind a front the core colour fades out, in cm.", -760, 200)
wave = graph.call(shock_rings, -460, 0, Position=world_xy(graph, -760, 0), WakeLength=wake_length,
                  FrontWidth=front_width)
finish_look(graph, wave, SHOCK_CORE_COLOR, SHOCK_EDGE_COLOR, SHOCK_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_PolygonRings",
                  "Glow layer of M_BackgroundLattice: the pulse rings as regular polygons, triangles by default, "
                  "born along the lattice and turning as they grow.")
sides = graph.parameter(scalar, "Sides", POLYGON_SIDES, "Shape", 0,
                        "Number of sides: 3 for triangles, 4 squares, 6 hexagons.", -760, 100)
twist = graph.parameter(scalar, "Twist", POLYGON_TWIST, "Shape", 1,
                        "Turn per metre a ring grows, in turns, counter-clockwise. 0 keeps them along the lattice.",
                        -760, 200)
ring_width = graph.parameter(scalar, "RingWidth", POLYGON_RING_WIDTH, "Shape", 2,
                             "Width of a ring at half brightness, in cm. Ring speed, reach and count live on "
                             "BP_GeoCam > BackgroundPulse.", -760, 300)
core_width = graph.parameter(scalar, "CoreWidth", POLYGON_CORE_WIDTH, "Shape", 3,
                             "Width of the core colour along a ring's middle, at half strength, in cm.", -760, 400)
wave = graph.call(polygon_rings, -460, 0, Position=world_xy(graph, -760, 0), Sides=sides, Twist=twist,
                  RingWidth=ring_width, CoreWidth=core_width)
finish_look(graph, wave, POLYGON_CORE_COLOR, POLYGON_EDGE_COLOR, POLYGON_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_Halos",
                  "Glow layer of M_BackgroundLattice: whole triangles lit around every pulse origin, under the "
                  "characters and deployables when BP_GeoCam > BackgroundPulse tracks actors, drifting when it "
                  "wanders.")
cell_size = graph.parameter(scalar, "CellSize", HALO_CELL_SIZE, "Shape", 0,
                            "Side length of the lattice's triangles, in cm: keep it equal to the pattern's "
                            "ShapeSize.", -760, 100)
halo_radius = graph.parameter(scalar, "Radius", HALO_RADIUS, "Shape", 1,
                              "How far a halo reaches, in cm. Half bright at half of it.", -760, 200)
core_radius = graph.parameter(scalar, "CoreRadius", HALO_CORE_RADIUS, "Shape", 2,
                              "How far a halo's core colour reaches, in cm.", -760, 300)
beat = graph.parameter(scalar, "Beat", HALO_BEAT, "Shape", 3,
                       "0 holds the halos steady; 1 flashes each with every ring it sends and fades it as the ring "
                       "grows.", -760, 400)
wave = graph.call(halos, -460, 0, Position=world_xy(graph, -760, 0), CellSize=cell_size, Radius=halo_radius,
                  CoreRadius=core_radius, Beat=beat)
finish_look(graph, wave, HALO_CORE_COLOR, HALO_EDGE_COLOR, HALO_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_Spiral",
                  "Glow layer of M_BackgroundLattice: spiral arms turning around the centre of the arena the floor "
                  "belongs to.")
arms = graph.parameter(scalar, "Arms", SPIRAL_ARMS, "Shape", 0, "Number of arms, a whole number.", -760, 100)
pitch = graph.parameter(scalar, "Pitch", SPIRAL_PITCH, "Shape", 1,
                        "Distance between neighbouring arms along a radius, in cm.", -760, 200)
turn_speed = graph.parameter(scalar, "TurnSpeed", SPIRAL_TURN_SPEED, "Shape", 2,
                             "Spin, in turns per second, counter-clockwise.", -960, 300)
width = graph.parameter(scalar, "Width", SPIRAL_WIDTH, "Shape", 3, "Width of an arm at half brightness, in cm.",
                        -760, 400)
core_width = graph.parameter(scalar, "CoreWidth", SPIRAL_CORE_WIDTH, "Shape", 4,
                             "Width of an arm's core colour at half strength, in cm.", -760, 500)
reach = graph.parameter(scalar, "Reach", SPIRAL_REACH, "Shape", 5,
                        "Distance from the centre where the arms have faded out, in cm.", -760, 600)
wave = graph.call(spiral, -460, 0, Position=world_xy(graph, -760, -100), Center=arena_xy(graph, -760, 0),
                  Arms=arms, Pitch=pitch, Turn=timed(graph, turn_speed, -760, 300), Width=width,
                  CoreWidth=core_width, Reach=reach)
finish_look(graph, wave, SPIRAL_CORE_COLOR, SPIRAL_EDGE_COLOR, SPIRAL_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_Whirl",
                  "Glow layer of M_BackgroundLattice: nested polygons around the arena's centre, each twice "
                  "the last, growing outward forever and twisting like a vortex.")
sides = graph.parameter(scalar, "Sides", WHIRL_SIDES, "Shape", 0,
                        "Number of sides: 3 for triangles, 4 squares, 6 hexagons.", -760, 100)
zoom_speed = graph.parameter(scalar, "ZoomSpeed", WHIRL_ZOOM_SPEED, "Shape", 1,
                             "Growth, in doublings per second. Negative falls inward.", -960, 200)
twist = graph.parameter(scalar, "Twist", WHIRL_TWIST, "Shape", 2,
                        "Turn between a polygon and the next one out, in turns. 0 nests them straight.", -760, 300)
turn_speed = graph.parameter(scalar, "TurnSpeed", WHIRL_TURN_SPEED, "Shape", 3,
                             "Spin of the whole whirl, in turns per second, counter-clockwise.", -960, 400)
width = graph.parameter(scalar, "Width", WHIRL_WIDTH, "Shape", 4, "Width of a line at half brightness, in cm.",
                        -760, 500)
core_width = graph.parameter(scalar, "CoreWidth", WHIRL_CORE_WIDTH, "Shape", 5,
                             "Width of a line's core colour at half strength, in cm.", -760, 600)
reach = graph.parameter(scalar, "Reach", WHIRL_REACH, "Shape", 6,
                        "Distance from the centre where the whirl has faded out, in cm.", -760, 700)
wave = graph.call(whirl, -460, 0, Position=world_xy(graph, -760, -100), Center=arena_xy(graph, -760, 0),
                  Sides=sides, Zoom=timed(graph, zoom_speed, -760, 200), Twist=twist,
                  Turn=timed(graph, turn_speed, -760, 400), Width=width, CoreWidth=core_width, Reach=reach)
finish_look(graph, wave, WHIRL_CORE_COLOR, WHIRL_EDGE_COLOR, WHIRL_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_Sierpinski",
                  "Glow layer of M_BackgroundLattice: the lattice's own triangles picked out as a Sierpinski triangle "
                  "32 rows high, repeating across the floor, a band printing it row by row.")
cell_size = graph.parameter(scalar, "CellSize", SIERPINSKI_CELL_SIZE, "Shape", 0,
                            "Side length of the lattice's triangles, in cm: keep it equal to the pattern's "
                            "ShapeSize.", -760, 100)
scan_speed = graph.parameter(scalar, "ScanSpeed", SIERPINSKI_SCAN_SPEED, "Shape", 1,
                             "Rows the band climbs per second. A row is ShapeSize * 0.87 cm.", -960, 200)
scan_width = graph.parameter(scalar, "ScanWidth", SIERPINSKI_SCAN_WIDTH, "Shape", 2,
                             "Width of the band at half brightness, in rows.", -760, 300)
core_width = graph.parameter(scalar, "CoreWidth", SIERPINSKI_CORE_WIDTH, "Shape", 3,
                             "Width of the band's core colour at half strength, in rows.", -760, 400)
rest = graph.parameter(scalar, "Rest", SIERPINSKI_REST, "Shape", 4,
                       "Brightness of the triangle away from the band, 0 to 1.", -760, 500)
wave = graph.call(sierpinski, -460, 0, Position=world_xy(graph, -760, 0), CellSize=cell_size,
                  Scan=timed(graph, scan_speed, -760, 200), ScanWidth=scan_width, CoreWidth=core_width, Rest=rest)
finish_look(graph, wave, SIERPINSKI_CORE_COLOR, SIERPINSKI_EDGE_COLOR, SIERPINSKI_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_Radar",
                  "Glow layer of M_BackgroundLattice: beams sweeping around the arena's centre, each with a hard "
                  "leading edge and a fading trail.")
beams = graph.parameter(scalar, "Beams", RADAR_BEAMS, "Shape", 0, "Number of beams, a whole number.", -760, 100)
turn_speed = graph.parameter(scalar, "TurnSpeed", RADAR_TURN_SPEED, "Shape", 1,
                             "Sweep speed, in turns per second, counter-clockwise.", -960, 200)
trail = graph.parameter(scalar, "Trail", RADAR_TRAIL, "Shape", 2,
                        "How far behind a beam its glow fades out, in turns.", -760, 300)
core_width = graph.parameter(scalar, "CoreWidth", RADAR_CORE_WIDTH, "Shape", 3,
                             "How far behind a beam its core colour fades out, in cm along the sweep.", -760, 400)
reach = graph.parameter(scalar, "Reach", RADAR_REACH, "Shape", 4,
                        "Distance from the centre where the sweep has faded out, in cm.", -760, 500)
wave = graph.call(radar, -460, 0, Position=world_xy(graph, -760, -100), Center=arena_xy(graph, -760, 0),
                  Beams=beams, Turn=timed(graph, turn_speed, -760, 200), Trail=trail, CoreWidth=core_width,
                  Reach=reach)
finish_look(graph, wave, RADAR_CORE_COLOR, RADAR_EDGE_COLOR, RADAR_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_Twinkle",
                  "Glow layer of M_BackgroundLattice: single triangles flashing at random all over the floor, each on "
                  "its own clock.")
cell_size = graph.parameter(scalar, "CellSize", TWINKLE_CELL_SIZE, "Shape", 0,
                            "Side length of the lattice's triangles, in cm: keep it equal to the pattern's "
                            "ShapeSize.", -760, 100)
rate = graph.parameter(scalar, "Rate", TWINKLE_RATE, "Shape", 1,
                       "Flashes per second of each triangle: 0.1 flashes each one every ten seconds.", -760, 200)
fade = graph.parameter(scalar, "Fade", TWINKLE_FADE, "Shape", 2, "Seconds a flash takes to fade out.", -760, 300)
wave = graph.call(twinkle, -460, 0, Position=world_xy(graph, -760, 0),
                  CellSize=cell_size, Time=graph.node(unreal.MaterialExpressionTime, -760, 150), Rate=rate, Fade=fade)
finish_look(graph, wave, TWINKLE_CORE_COLOR, TWINKLE_EDGE_COLOR, TWINKLE_BRIGHTNESS, -100, 0)

graph = open_look("ML_Wave_Fireflies",
                  "Glow layer of M_BackgroundLattice: five lights drifting over the floor around the arena's centre "
                  "on slow, never-quite-repeating paths, lighting the triangles under them.")
cell_size = graph.parameter(scalar, "CellSize", FIREFLY_CELL_SIZE, "Shape", 0,
                            "Side length of the lattice's triangles, in cm: keep it equal to the pattern's "
                            "ShapeSize.", -760, 100)
spread = graph.parameter(scalar, "Spread", FIREFLY_SPREAD, "Shape", 1,
                         "Furthest a light wanders from the arena's centre along each axis, in cm.", -760, 200)
speed = graph.parameter(scalar, "Speed", FIREFLY_SPEED, "Shape", 2,
                        "Travel speed, 1 taking each light about 20 s to swing across and back.", -960, 300)
halo_radius = graph.parameter(scalar, "Radius", FIREFLY_RADIUS, "Shape", 3,
                              "How far a light reaches, in cm. Half bright at half of it.", -760, 400)
core_radius = graph.parameter(scalar, "CoreRadius", FIREFLY_CORE_RADIUS, "Shape", 4,
                              "How far a light's core colour reaches, in cm.", -760, 500)
wave = graph.call(fireflies, -460, 0, Position=world_xy(graph, -760, -100), Center=arena_xy(graph, -760, 0),
                  CellSize=cell_size, Spread=spread, Time=timed(graph, speed, -760, 300), Radius=halo_radius,
                  CoreRadius=core_radius)
finish_look(graph, wave, FIREFLY_CORE_COLOR, FIREFLY_EDGE_COLOR, FIREFLY_BRIGHTNESS, -100, 0)

unreal.log(f"BGLOOKS::built {LAYERS}")
