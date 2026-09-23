"""Standard material functions any material calls: distances to shapes, tiling cells, strokes, two-tone glows, masks."""
import unreal

FOLDER = "/Game/Art/VFX/Generic/Materials/Functions"
CATEGORY = "GeoTrinity|Math"
SQRT3_OVER_2 = 0.8660254037844386
ONE_OVER_TWO_PI = 0.15915494309189535

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
open_function = toolkit["open_function"]

# --- MF_RepeatDistance: |frac(Value / Spacing + 0.5) - 0.5| spacings to the nearest multiple ---
graph = open_function(FOLDER, "MF_RepeatDistance",
                      "Distance from a value to the nearest whole multiple of Spacing.", CATEGORY)
value = graph.input("Value", "Scalar", 0, "Value to measure from.", -840, -40)
spacing = graph.input("Spacing", "Scalar", 1, "Distance between neighbouring multiples, in the units of Value.",
                      -840, 80)
in_spacings = graph.op(unreal.MaterialExpressionDivide, -620, 0, A=value, B=spacing)
# frac(x + 0.5) - 0.5: x's offset from the nearest whole number.
shifted = graph.node(unreal.MaterialExpressionAdd, -460, 0, const_b=0.5)
graph.connect(in_spacings, shifted, "A")
wrapped = graph.op(unreal.MaterialExpressionFrac, -320, 0, shifted)
centred = graph.node(unreal.MaterialExpressionSubtract, -200, 0, const_b=0.5)
graph.connect(wrapped, centred, "A")
offset = graph.op(unreal.MaterialExpressionAbs, -60, 0, centred)
distance = graph.op(unreal.MaterialExpressionMultiply, 60, 40, A=offset, B=spacing)
graph.output("Distance", 0, "Distance to the nearest multiple, in the units of Value.", distance, 220, 40)
graph.finish()
repeat_distance = graph.asset

# --- MF_PolarCoordinates: length and atan2 in turns ---
graph = open_function(FOLDER, "MF_PolarCoordinates", "Distance and angle of a point around the origin.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to locate, relative to the centre.", -760, 0)
radius = graph.op(unreal.MaterialExpressionLength, -500, -100, position)
position_x = graph.mask(position, "r", -520, 20)
position_y = graph.mask(position, "g", -520, 100)
angle = graph.op(unreal.MaterialExpressionArctangent2, -360, 60, Y=position_y, X=position_x)
angle_turns = graph.node(unreal.MaterialExpressionMultiply, -200, 60, const_b=ONE_OVER_TWO_PI)
graph.connect(angle, angle_turns, "A")
graph.output("Radius", 0, "Distance from the centre, in the units of Position.", radius, -40, -100)
graph.output("Angle", 1, "Counter-clockwise angle from +X, in turns, -0.5 to 0.5.", angle_turns, -40, 60)
graph.finish()
polar_coordinates = graph.asset

# --- MF_ParallelLinesDistance: the offset along the normal, to the nearest multiple of the spacing ---
graph = open_function(FOLDER, "MF_ParallelLinesDistance",
                      "Distance to the nearest of an endless set of evenly spaced parallel lines, one of them through "
                      "the origin.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to measure from.", -760, -80)
normal = graph.input("Normal", "Vector2", 1, "Unit vector perpendicular to the lines.", -760, 40)
spacing = graph.input("Spacing", "Scalar", 2, "Distance between neighbouring lines, in the units of Position.",
                      -760, 160)
along = graph.op(unreal.MaterialExpressionDotProduct, -520, -40, A=position, B=normal)
to_line = graph.call(repeat_distance, -340, 40, Value=along, Spacing=spacing)
graph.output("Distance", 0, "Distance to the nearest line, in the units of Position.", (to_line, "Distance"), -80, 40)
graph.finish()

# --- MF_CircleDistance: | |Position - Center| - Radius | ---
graph = open_function(FOLDER, "MF_CircleDistance",
                      "Distance to the outline of a circle: zero on it, growing both inside and outside.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to measure from.", -840, -60)
center = graph.input("Center", "Vector2", 1, "Centre of the circle.", -840, 60)
radius = graph.input("Radius", "Scalar", 2, "Radius of the circle, in the units of Position.", -840, 180)
offset = graph.op(unreal.MaterialExpressionSubtract, -600, 0, A=position, B=center)
from_center = graph.op(unreal.MaterialExpressionLength, -440, 0, offset)
from_outline = graph.op(unreal.MaterialExpressionSubtract, -300, 60, A=from_center, B=radius)
distance = graph.op(unreal.MaterialExpressionAbs, -140, 60, from_outline)
graph.output("Distance", 0, "Distance to the outline, in the units of Position.", distance, 20, 60)
graph.finish()

# --- MF_PolygonDistance: along the nearest edge normal, minus the apothem ---
graph = open_function(FOLDER, "MF_PolygonDistance",
                      "Distance to the outline of a regular polygon centred on the origin, with sharp corners. At "
                      "Rotation 0 it has a flat edge facing +X.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to measure from, relative to the polygon's centre.", -1300, -40)
sides = graph.input("Sides", "Scalar", 1, "Number of sides, 3 or more.", -1300, 200)
radius = graph.input("Radius", "Scalar", 2, "Distance from the centre to the corners, in the units of Position.",
                     -1300, 420)
rotation = graph.input("Rotation", "Scalar", 3, "Counter-clockwise turn, in turns.", -1300, 540)
polar = graph.call(polar_coordinates, -1060, -20, Position=position)
turned = graph.op(unreal.MaterialExpressionSubtract, -780, 120, A=(polar, "Angle"), B=rotation)
in_sectors = graph.op(unreal.MaterialExpressionMultiply, -620, 160, A=turned, B=sides)
sector = graph.op(unreal.MaterialExpressionRound, -480, 160, in_sectors)
nearest_normal = graph.op(unreal.MaterialExpressionDivide, -340, 200, A=sector, B=sides)
off_normal = graph.op(unreal.MaterialExpressionSubtract, -200, 120, A=nearest_normal, B=turned)
# Cosine's default period of 1 reads its input in turns.
off_normal_cosine = graph.op(unreal.MaterialExpressionCosine, -60, 120, off_normal)
along_normal = graph.op(unreal.MaterialExpressionMultiply, 80, 0, A=(polar, "Radius"), B=off_normal_cosine)
half_sector = graph.node(unreal.MaterialExpressionDivide, -620, 340, const_a=0.5)
graph.connect(sides, half_sector, "B")
half_sector_cosine = graph.op(unreal.MaterialExpressionCosine, -480, 340, half_sector)
apothem = graph.op(unreal.MaterialExpressionMultiply, -300, 400, A=radius, B=half_sector_cosine)
from_edge = graph.op(unreal.MaterialExpressionSubtract, 220, 160, A=along_normal, B=apothem)
distance = graph.op(unreal.MaterialExpressionAbs, 360, 160, from_edge)
graph.output("Distance", 0, "Distance to the outline, in the units of Position.", distance, 500, 160)
graph.finish()

# --- MF_TriangleCell: the two edge sets' fractional rows locate the point inside its triangle ---
graph = open_function(FOLDER, "MF_TriangleCell",
                      "Where a point sits inside its triangle, where that triangle's centre is and which one it is, in "
                      "a tiling of "
                      "equilateral triangles with Size-long sides, a corner on the origin and edges facing +Y and -Y: "
                      "the tiling MF_ParallelLinesDistance draws with normals (0,1), (-sqrt3/2,1/2), (sqrt3/2,1/2) and "
                      "spacing Size*sqrt3/2.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to locate.", -1500, 0)
size = graph.input("Size", "Scalar", 1, "Side length of the triangles, in the units of Position.", -1500, 400)
spacing = graph.node(unreal.MaterialExpressionMultiply, -1260, 400, const_b=SQRT3_OVER_2)
graph.connect(size, spacing, "A")
rows = []
whole_rows = []
for index, normal_x in enumerate((-SQRT3_OVER_2, SQRT3_OVER_2)):
    row = -120 + index * 200
    normal = graph.node(unreal.MaterialExpressionConstant2Vector, -1260, row + 60, r=normal_x, g=0.5)
    along = graph.op(unreal.MaterialExpressionDotProduct, -1080, row, A=position, B=normal)
    in_rows = graph.op(unreal.MaterialExpressionDivide, -920, row + 40, A=along, B=spacing)
    rows.append(graph.op(unreal.MaterialExpressionFrac, -780, row + 40, in_rows))
    whole_rows.append(graph.op(unreal.MaterialExpressionFloor, -780, row + 120, in_rows))

row_sum = graph.op(unreal.MaterialExpressionAdd, -640, 0, A=rows[0], B=rows[1])
# 1 in the triangles pointing the other way, whose centre sits at 2/3 of both rows instead of 1/3.
flipped = graph.node(unreal.MaterialExpressionStep, -500, 120, const_y=1.0)
graph.connect(row_sum, flipped, "X")
flipped_share = graph.node(unreal.MaterialExpressionMultiply, -360, 160, const_b=2.0 / 3.0)
graph.connect(flipped, flipped_share, "A")
twice_center = graph.node(unreal.MaterialExpressionAdd, -220, 160, const_b=2.0 / 3.0)
graph.connect(flipped_share, twice_center, "A")
toward_flat_edge = graph.op(unreal.MaterialExpressionSubtract, -80, 0, A=row_sum, B=twice_center)
local_x = graph.op(unreal.MaterialExpressionMultiply, 60, 60, A=toward_flat_edge, B=spacing)
sideways = graph.op(unreal.MaterialExpressionSubtract, -640, -160, A=rows[0], B=rows[1])
half_size = graph.node(unreal.MaterialExpressionMultiply, -1260, 520, const_b=0.5)
graph.connect(size, half_size, "A")
local_y = graph.op(unreal.MaterialExpressionMultiply, 60, -120, A=sideways, B=half_size)
local = graph.op(unreal.MaterialExpressionAppendVector, 220, -40, A=local_x, B=local_y)
# A half turn maps the flipped triangles onto the others, so every cell faces +X the same way.
flip_scale = graph.node(unreal.MaterialExpressionMultiply, -80, 240, const_b=-2.0)
graph.connect(flipped, flip_scale, "A")
flip_sign = graph.node(unreal.MaterialExpressionAdd, 60, 240, const_b=1.0)
graph.connect(flip_scale, flip_sign, "A")
cell_position = graph.op(unreal.MaterialExpressionMultiply, 380, 40, A=local, B=flip_sign)
graph.output("CellPosition", 0, "Position from the centre of the point's triangle, in the units of Position, turned so "
             "every triangle has a flat edge facing +X like MF_PolygonDistance at Rotation 0.", cell_position, 540, 40)
# local turned back a quarter turn is the point's offset from the centre in Position's own axes.
negative_local_y = graph.node(unreal.MaterialExpressionMultiply, 220, -220, const_b=-1.0)
graph.connect(local_y, negative_local_y, "A")
offset = graph.op(unreal.MaterialExpressionAppendVector, 380, -180, A=negative_local_y, B=local_x)
cell_center = graph.op(unreal.MaterialExpressionSubtract, 540, -140, A=position, B=offset)
graph.output("CellCenter", 1, "Centre of the point's triangle, in the units of Position: one value for the whole "
             "triangle.", cell_center, 700, -140)
whole = graph.op(unreal.MaterialExpressionAppendVector, 540, 320, A=whole_rows[0], B=whole_rows[1])
cell_index = graph.op(unreal.MaterialExpressionAppendVector, 700, 320, A=whole, B=flipped)
graph.output("CellIndex", 2, "Whole-number coordinates of the point's triangle: its row in each slanted edge set, then "
             "1 for a triangle pointing +Y, 0 for one pointing -Y.", cell_index, 860, 320)
graph.finish()

# --- MF_DoubleLineDistance: |Distance - Gap / 2| ---
graph = open_function(FOLDER, "MF_DoubleLineDistance",
                      "Distance to two copies of a path, Gap apart, one either side of it. Drawn with a stroke it "
                      "doubles a line.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path.", -620, -40)
gap = graph.input("Gap", "Scalar", 1, "Distance between the two copies, centre to centre, in the units of Distance. 0 "
                  "leaves a single path.", -620, 80)
half_gap = graph.node(unreal.MaterialExpressionMultiply, -400, 80, const_b=0.5)
graph.connect(gap, half_gap, "A")
from_copy = graph.op(unreal.MaterialExpressionSubtract, -240, 0, A=distance, B=half_gap)
to_copy = graph.op(unreal.MaterialExpressionAbs, -100, 0, from_copy)
graph.output("Distance", 0, "Distance to the nearest copy, in the units of Distance.", to_copy, 40, 0)
graph.finish()

# --- MF_Stroke_Hard: 1 within half a width of the path ---
graph = open_function(FOLDER, "MF_Stroke_Hard",
                      "Hard-edged line of a given width, drawn along the path a distance measures to.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path, such as MF_ParallelLinesDistance's.", -620, -40)
width = graph.input("Width", "Scalar", 1, "Full width of the line, in the units of Distance.", -620, 80)
half_width = graph.node(unreal.MaterialExpressionMultiply, -400, 80, const_b=0.5)
graph.connect(width, half_width, "A")
# Step(Y, X) is 1 where X >= Y.
mask = graph.op(unreal.MaterialExpressionStep, -240, 0, Y=distance, X=half_width)
graph.output("Mask", 0, "1 on the line, 0 off it.", mask, -60, 0)
graph.finish()

# --- MF_Stroke_Linear: 1 on the path, 0.5 at half a width, 0 at a width ---
graph = open_function(FOLDER, "MF_Stroke_Linear",
                      "Soft line drawn along the path a distance measures to, fading linearly away from it. At half "
                      "brightness it is as wide as MF_Stroke_Hard, so the two swap.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path, such as MF_CircleDistance's.", -700, -40)
width = graph.input("Width", "Scalar", 1, "Width of the line at half brightness, in the units of Distance. It fades "
                    "out completely this far from the path.", -700, 80)
in_widths = graph.op(unreal.MaterialExpressionDivide, -480, 0, A=distance, B=width)
fade = graph.op(unreal.MaterialExpressionOneMinus, -320, 0, in_widths)
mask = graph.op(unreal.MaterialExpressionSaturate, -180, 0, fade)
graph.output("Mask", 0, "1 on the path, fading to 0 at Width away.", mask, -40, 0)
graph.finish()

# --- MF_Stroke_Smooth: 1 - smoothstep(0, Width, Distance) ---
graph = open_function(FOLDER, "MF_Stroke_Smooth",
                      "Soft line drawn along the path a distance measures to, fading away from it with rounded "
                      "shoulders. At half brightness it is as wide as MF_Stroke_Hard, so the strokes swap.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path, such as MF_CircleDistance's.", -700, -40)
width = graph.input("Width", "Scalar", 1, "Width of the line at half brightness, in the units of Distance. It fades "
                    "out completely this far from the path.", -700, 80)
fade = graph.node(unreal.MaterialExpressionSmoothStep, -460, 0, const_min=0.0)
graph.connect(width, fade, "Max")
graph.connect(distance, fade, "Value")
mask = graph.op(unreal.MaterialExpressionOneMinus, -300, 0, fade)
graph.output("Mask", 0, "1 on the path, easing to 0 at Width away.", mask, -160, 0)
graph.finish()
stroke_smooth = graph.asset

# --- MF_TwoToneStroke: two smooth strokes of one distance, a wide glow and a narrow core ---
graph = open_function(FOLDER, "MF_TwoToneStroke",
                      "A soft line drawn twice over one distance: a wide Glow and a narrow Core inside it, both scaled "
                      "by Intensity. MF_TwoToneColor colours the pair.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path.", -760, -80)
width = graph.input("Width", "Scalar", 1, "Width of the glow at half brightness, in the units of Distance.", -760, 40)
core_width = graph.input("CoreWidth", "Scalar", 2, "Width of the core at half brightness, in the units of Distance. "
                         "Narrower than Width.", -760, 160)
intensity = graph.input("Intensity", "Scalar", 3, "Scales both, 0 to 1.", -760, 280)
glow_line = graph.call(stroke_smooth, -520, -60, Distance=distance, Width=width)
core_line = graph.call(stroke_smooth, -520, 120, Distance=distance, Width=core_width)
glow = graph.op(unreal.MaterialExpressionMultiply, -240, -20, A=(glow_line, "Mask"), B=intensity)
core = graph.op(unreal.MaterialExpressionMultiply, -240, 160, A=(core_line, "Mask"), B=intensity)
graph.output("Glow", 0, "Brightness of the line, 0 to 1.", glow, -80, -20)
graph.output("Core", 1, "The part of Glow in the core, 0 to Glow.", core, -80, 160)
graph.finish()

# --- MF_TwoToneColor: EdgeColor * (Glow - Core) + CoreColor * Core, times Brightness ---
graph = open_function(FOLDER, "MF_TwoToneColor",
                      "Colours a glow in two tones: its Core share in CoreColor, the rest in EdgeColor, all times "
                      "Brightness.", CATEGORY)
glow = graph.input("Glow", "Scalar", 0, "Brightness, 0 to 1.", -760, -120)
core = graph.input("Core", "Scalar", 1, "The part of Glow drawn in CoreColor. Anything above Glow counts as Glow.",
                   -760, 0)
core_color = graph.input("CoreColor", "Vector3", 2, "Colour of the core share.", -760, 120)
edge_color = graph.input("EdgeColor", "Vector3", 3, "Colour of the rest of the glow.", -760, 240)
brightness = graph.input("Brightness", "Scalar", 4, "Multiplies the result.", -760, 360)
core_share = graph.op(unreal.MaterialExpressionMin, -540, -40, A=core, B=glow)
edge_share = graph.op(unreal.MaterialExpressionSubtract, -400, -120, A=glow, B=core_share)
edge_light = graph.op(unreal.MaterialExpressionMultiply, -240, 160, A=edge_color, B=edge_share)
core_light = graph.op(unreal.MaterialExpressionMultiply, -240, 20, A=core_color, B=core_share)
light = graph.op(unreal.MaterialExpressionAdd, -100, 80, A=core_light, B=edge_light)
color = graph.op(unreal.MaterialExpressionMultiply, 40, 160, A=light, B=brightness)
graph.output("Color", 0, "Linear colour, for emissive.", color, 180, 160)
graph.finish()

# --- MF_SharedBit: frac(x / Period) >= 0.5 and frac(y / Period) >= 0.5 ---
graph = open_function(FOLDER, "MF_SharedBit",
                      "1 where the whole parts of both coordinates have one binary digit set: the digit worth half of "
                      "Period, a power of two.", CATEGORY)
coordinates = graph.input("Coordinates", "Vector2", 0, "The two values to test.", -760, -40)
period = graph.input("Period", "Scalar", 1, "Twice the digit's worth: 2 tests the ones, 4 the twos, and so on.", -760,
                     80)
in_periods = graph.op(unreal.MaterialExpressionDivide, -560, 0, A=coordinates, B=period)
wrapped = graph.op(unreal.MaterialExpressionFrac, -420, 0, in_periods)
# Step(Y, X) is 1 where X >= Y: the upper half of every period has the digit set.
digits = graph.node(unreal.MaterialExpressionStep, -300, 0, const_y=0.5)
graph.connect(wrapped, digits, "X")
first = graph.mask(digits, "r", -160, -40)
second = graph.mask(digits, "g", -160, 40)
both = graph.op(unreal.MaterialExpressionMultiply, -20, 0, A=first, B=second)
graph.output("Mask", 0, "1 where both have the digit, 0 elsewhere.", both, 120, 0)
graph.finish()
shared_bit = graph.asset

# --- MF_SierpinskiMask: no binary digit shared over five digits ---
graph = open_function(FOLDER, "MF_SierpinskiMask",
                      "1 on the Sierpinski triangle: where the whole parts of the two coordinates share no binary "
                      "digit, which is Pascal's triangle taken modulo 2. Five digits are tested, so the pattern "
                      "repeats every 32 along both coordinates.", CATEGORY)
coordinates = graph.input("Coordinates", "Vector2", 0, "Position along the two sides the triangle spans, in cells.",
                          -900, 0)
holes = None
for digit in range(5):
    row = -320 + digit * 160
    period = graph.node(unreal.MaterialExpressionConstant, -700, row + 60, r=2.0 ** (digit + 1))
    shared = graph.call(shared_bit, -540, row, Coordinates=coordinates, Period=period)
    if holes is None:
        holes = (shared, "Mask")
    else:
        holes = graph.op(unreal.MaterialExpressionMax, -300, row, A=holes, B=(shared, "Mask"))

mask = graph.op(unreal.MaterialExpressionOneMinus, -160, 320, holes)
graph.output("Mask", 0, "1 on the triangle, 0 in its holes.", mask, -20, 320)
graph.finish()

# --- MF_LissajousPoint: Center + Spread * sin(Frequency * Time + Phase), per axis ---
graph = open_function(FOLDER, "MF_LissajousPoint",
                      "A point travelling a Lissajous figure around a centre: each axis swings back and forth on its "
                      "own frequency, so unrelated frequencies trace a path that takes long to repeat.", CATEGORY)
center = graph.input("Center", "Vector2", 0, "Middle of the figure.", -760, -120)
spread = graph.input("Spread", "Scalar", 1, "Furthest the point goes from Center along each axis, in the units of "
                     "Center.", -760, 0)
time = graph.input("Time", "Scalar", 2, "Seconds, or any clock.", -760, 120)
frequency = graph.input("Frequency", "Vector2", 3, "Swings per second along X and along Y.", -760, 240)
phase = graph.input("Phase", "Vector2", 4, "Start of each swing, in turns.", -760, 360)
swings = graph.op(unreal.MaterialExpressionMultiply, -540, 180, A=frequency, B=time)
shifted = graph.op(unreal.MaterialExpressionAdd, -400, 220, A=swings, B=phase)
# Sine's default period of 1 reads its input in turns.
sway = graph.op(unreal.MaterialExpressionSine, -280, 220, shifted)
offset = graph.op(unreal.MaterialExpressionMultiply, -160, 120, A=sway, B=spread)
point = graph.op(unreal.MaterialExpressionAdd, -20, 0, A=center, B=offset)
graph.output("Point", 0, "Where the point is now, in the units of Center.", point, 120, 0)
graph.finish()

# --- MF_RandomFromPosition: frac(sin(dot(Position, (12.9898, 78.233))) * 43758.5453) ---
graph = open_function(FOLDER, "MF_RandomFromPosition",
                      "A repeatable pseudo-random value from 0 to 1 for a position: the same position always gives "
                      "the same value, neighbouring ones unrelated values. Keep the position within a few thousand "
                      "units, such as cells rather than cm, or the values lose their spread.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Position to draw the value for, such as a cell's centre in cells.",
                       -760, 0)
key = graph.node(unreal.MaterialExpressionConstant2Vector, -760, 120, r=12.9898, g=78.233)
seed = graph.op(unreal.MaterialExpressionDotProduct, -560, 40, A=position, B=key)
# Sine reads its input in turns; the period changes nothing for a hash.
wave = graph.op(unreal.MaterialExpressionSine, -420, 40, seed)
spread = graph.node(unreal.MaterialExpressionMultiply, -280, 40, const_b=43758.5453)
graph.connect(wave, spread, "A")
value = graph.op(unreal.MaterialExpressionFrac, -140, 40, spread)
graph.output("Value", 0, "0 to 1.", value, 0, 40)
graph.finish()

# --- MF_DurationWipe: Step(frac(atan2(y, x) / 2pi + 0.25), 1 - Spent) over the quad centred to [-1, 1] ---
graph = open_function(FOLDER, "MF_DurationWipe",
                      "Clock-wipe mask for a remaining-life readout: the wedge of a disc still alive once Spent of it "
                      "is used up, emptying clockwise from 12 o'clock.", CATEGORY)
uv = graph.input("UV", "Vector2", 0, "Texture coordinates of the disc's quad.", -1400, -100)
spent = graph.input("Spent", "Scalar", 1, "Fraction used up, 0 to 1. At 0 the disc is full, so an unwritten custom "
                    "primitive data slot reads as untouched.", -1400, 220)
centered = graph.node(unreal.MaterialExpressionSubtract, -1180, -100, const_b=0.5)
graph.connect(uv, centered, "A")
quad = graph.node(unreal.MaterialExpressionMultiply, -1000, -100, const_b=2.0)
graph.connect(centered, quad, "A")
# UV y grows downward, which makes the angle run clockwise on screen.
polar = graph.call(polar_coordinates, -760, -100, Position=quad)
from_noon = graph.node(unreal.MaterialExpressionAdd, -240, -100, const_b=0.25)
graph.connect((polar, "Angle"), from_noon, "A")
# atan2 spans half a turn either side of 0; frac wraps the negative arc onto the top of the range.
turn = graph.op(unreal.MaterialExpressionFrac, -60, -100, from_noon)
alive = graph.op(unreal.MaterialExpressionOneMinus, -240, 220, spent)
# Step(Y, X) is 1 where X >= Y.
wipe = graph.op(unreal.MaterialExpressionStep, 140, 0, Y=turn, X=alive)
graph.output("Wipe", 0, "1 on the wedge still alive, 0 on the spent part.", wipe, 380, 0)
graph.finish()

unreal.log(f"GENERICFUNCTIONS::built {FOLDER}")
