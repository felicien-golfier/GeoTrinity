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
value = graph.input("Value", "Scalar", 0, "Value to measure from.")
spacing = graph.input("Spacing", "Scalar", 1, "Distance between neighbouring multiples, in the units of Value.")
in_spacings = graph.op(unreal.MaterialExpressionDivide, A=value, B=spacing)
# frac(x + 0.5) - 0.5: x's offset from the nearest whole number.
shifted = graph.node(unreal.MaterialExpressionAdd, const_b=0.5)
graph.connect(in_spacings, shifted, "A")
wrapped = graph.op(unreal.MaterialExpressionFrac, shifted)
centred = graph.node(unreal.MaterialExpressionSubtract, const_b=0.5)
graph.connect(wrapped, centred, "A")
offset = graph.op(unreal.MaterialExpressionAbs, centred)
distance = graph.op(unreal.MaterialExpressionMultiply, A=offset, B=spacing)
graph.output("Distance", 0, "Distance to the nearest multiple, in the units of Value.", distance)
graph.finish()
repeat_distance = graph.asset

# --- MF_PolarCoordinates: length and atan2 in turns ---
graph = open_function(FOLDER, "MF_PolarCoordinates", "Distance and angle of a point around the origin.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to locate, relative to the centre.")
radius = graph.op(unreal.MaterialExpressionLength, position)
position_x = graph.mask(position, "r")
position_y = graph.mask(position, "g")
angle = graph.op(unreal.MaterialExpressionArctangent2, Y=position_y, X=position_x)
angle_turns = graph.node(unreal.MaterialExpressionMultiply, const_b=ONE_OVER_TWO_PI)
graph.connect(angle, angle_turns, "A")
graph.output("Radius", 0, "Distance from the centre, in the units of Position.", radius)
graph.output("Angle", 1, "Counter-clockwise angle from +X, in turns, -0.5 to 0.5.", angle_turns)
graph.finish()
polar_coordinates = graph.asset

# --- MF_ParallelLinesDistance: the offset along the normal, to the nearest multiple of the spacing ---
graph = open_function(FOLDER, "MF_ParallelLinesDistance",
                      "Distance to the nearest of an endless set of evenly spaced parallel lines, one of them through "
                      "the origin.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to measure from.")
normal = graph.input("Normal", "Vector2", 1, "Unit vector perpendicular to the lines.")
spacing = graph.input("Spacing", "Scalar", 2, "Distance between neighbouring lines, in the units of Position.")
along = graph.op(unreal.MaterialExpressionDotProduct, A=position, B=normal)
to_line = graph.call(repeat_distance, Value=along, Spacing=spacing)
graph.output("Distance", 0, "Distance to the nearest line, in the units of Position.", (to_line, "Distance"))
graph.finish()

# --- MF_CircleDistance: | |Position - Center| - Radius | ---
graph = open_function(FOLDER, "MF_CircleDistance",
                      "Distance to the outline of a circle: zero on it, growing both inside and outside.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to measure from.")
center = graph.input("Center", "Vector2", 1, "Centre of the circle.")
radius = graph.input("Radius", "Scalar", 2, "Radius of the circle, in the units of Position.")
offset = graph.op(unreal.MaterialExpressionSubtract, A=position, B=center)
from_center = graph.op(unreal.MaterialExpressionLength, offset)
from_outline = graph.op(unreal.MaterialExpressionSubtract, A=from_center, B=radius)
distance = graph.op(unreal.MaterialExpressionAbs, from_outline)
graph.output("Distance", 0, "Distance to the outline, in the units of Position.", distance)
graph.finish()

# --- MF_PolygonDistance: along the nearest edge normal, minus the apothem ---
graph = open_function(FOLDER, "MF_PolygonDistance",
                      "Distance to the outline of a regular polygon centred on the origin, with sharp corners. At "
                      "Rotation 0 it has a flat edge facing +X.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to measure from, relative to the polygon's centre.")
sides = graph.input("Sides", "Scalar", 1, "Number of sides, 3 or more.")
radius = graph.input("Radius", "Scalar", 2, "Distance from the centre to the corners, in the units of Position.")
rotation = graph.input("Rotation", "Scalar", 3, "Counter-clockwise turn, in turns.")
polar = graph.call(polar_coordinates, Position=position)
turned = graph.op(unreal.MaterialExpressionSubtract, A=(polar, "Angle"), B=rotation)
in_sectors = graph.op(unreal.MaterialExpressionMultiply, A=turned, B=sides)
sector = graph.op(unreal.MaterialExpressionRound, in_sectors)
nearest_normal = graph.op(unreal.MaterialExpressionDivide, A=sector, B=sides)
off_normal = graph.op(unreal.MaterialExpressionSubtract, A=nearest_normal, B=turned)
# Cosine's default period of 1 reads its input in turns.
off_normal_cosine = graph.op(unreal.MaterialExpressionCosine, off_normal)
along_normal = graph.op(unreal.MaterialExpressionMultiply, A=(polar, "Radius"), B=off_normal_cosine)
half_sector = graph.node(unreal.MaterialExpressionDivide, const_a=0.5)
graph.connect(sides, half_sector, "B")
half_sector_cosine = graph.op(unreal.MaterialExpressionCosine, half_sector)
apothem = graph.op(unreal.MaterialExpressionMultiply, A=radius, B=half_sector_cosine)
from_edge = graph.op(unreal.MaterialExpressionSubtract, A=along_normal, B=apothem)
distance = graph.op(unreal.MaterialExpressionAbs, from_edge)
graph.output("Distance", 0, "Distance to the outline, in the units of Position.", distance)
graph.finish()

# --- MF_TriangleCell: the two edge sets' fractional rows locate the point inside its triangle ---
graph = open_function(FOLDER, "MF_TriangleCell",
                      "Where a point sits inside its triangle, where that triangle's centre is and which one it is, in "
                      "a tiling of "
                      "equilateral triangles with Size-long sides, a corner on the origin and edges facing +Y and -Y: "
                      "the tiling MF_ParallelLinesDistance draws with normals (0,1), (-sqrt3/2,1/2), (sqrt3/2,1/2) and "
                      "spacing Size*sqrt3/2.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to locate.")
size = graph.input("Size", "Scalar", 1, "Side length of the triangles, in the units of Position.")
spacing = graph.node(unreal.MaterialExpressionMultiply, const_b=SQRT3_OVER_2)
graph.connect(size, spacing, "A")
rows = []
whole_rows = []
for normal_x in (-SQRT3_OVER_2, SQRT3_OVER_2):
    normal = graph.node(unreal.MaterialExpressionConstant2Vector, r=normal_x, g=0.5)
    along = graph.op(unreal.MaterialExpressionDotProduct, A=position, B=normal)
    in_rows = graph.op(unreal.MaterialExpressionDivide, A=along, B=spacing)
    rows.append(graph.op(unreal.MaterialExpressionFrac, in_rows))
    whole_rows.append(graph.op(unreal.MaterialExpressionFloor, in_rows))

row_sum = graph.op(unreal.MaterialExpressionAdd, A=rows[0], B=rows[1])
# 1 in the triangles pointing the other way, whose centre sits at 2/3 of both rows instead of 1/3.
flipped = graph.node(unreal.MaterialExpressionStep, const_y=1.0)
graph.connect(row_sum, flipped, "X")
flipped_share = graph.node(unreal.MaterialExpressionMultiply, const_b=2.0 / 3.0)
graph.connect(flipped, flipped_share, "A")
twice_center = graph.node(unreal.MaterialExpressionAdd, const_b=2.0 / 3.0)
graph.connect(flipped_share, twice_center, "A")
toward_flat_edge = graph.op(unreal.MaterialExpressionSubtract, A=row_sum, B=twice_center)
local_x = graph.op(unreal.MaterialExpressionMultiply, A=toward_flat_edge, B=spacing)
sideways = graph.op(unreal.MaterialExpressionSubtract, A=rows[0], B=rows[1])
half_size = graph.node(unreal.MaterialExpressionMultiply, const_b=0.5)
graph.connect(size, half_size, "A")
local_y = graph.op(unreal.MaterialExpressionMultiply, A=sideways, B=half_size)
local = graph.op(unreal.MaterialExpressionAppendVector, A=local_x, B=local_y)
# A half turn maps the flipped triangles onto the others, so every cell faces +X the same way.
flip_scale = graph.node(unreal.MaterialExpressionMultiply, const_b=-2.0)
graph.connect(flipped, flip_scale, "A")
flip_sign = graph.node(unreal.MaterialExpressionAdd, const_b=1.0)
graph.connect(flip_scale, flip_sign, "A")
cell_position = graph.op(unreal.MaterialExpressionMultiply, A=local, B=flip_sign)
graph.output("CellPosition", 0, "Position from the centre of the point's triangle, in the units of Position, turned so "
             "every triangle has a flat edge facing +X like MF_PolygonDistance at Rotation 0.", cell_position)
# local turned back a quarter turn is the point's offset from the centre in Position's own axes.
negative_local_y = graph.node(unreal.MaterialExpressionMultiply, const_b=-1.0)
graph.connect(local_y, negative_local_y, "A")
offset = graph.op(unreal.MaterialExpressionAppendVector, A=negative_local_y, B=local_x)
cell_center = graph.op(unreal.MaterialExpressionSubtract, A=position, B=offset)
graph.output("CellCenter", 1, "Centre of the point's triangle, in the units of Position: one value for the whole "
             "triangle.", cell_center)
whole = graph.op(unreal.MaterialExpressionAppendVector, A=whole_rows[0], B=whole_rows[1])
cell_index = graph.op(unreal.MaterialExpressionAppendVector, A=whole, B=flipped)
graph.output("CellIndex", 2, "Whole-number coordinates of the point's triangle: its row in each slanted edge set, then "
             "1 for a triangle pointing +Y, 0 for one pointing -Y.", cell_index)
graph.finish()

# --- MF_DoubleLineDistance: |Distance - Gap / 2| ---
graph = open_function(FOLDER, "MF_DoubleLineDistance",
                      "Distance to two copies of a path, Gap apart, one either side of it. Drawn with a stroke it "
                      "doubles a line.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path.")
gap = graph.input("Gap", "Scalar", 1, "Distance between the two copies, centre to centre, in the units of Distance. 0 "
                  "leaves a single path.")
half_gap = graph.node(unreal.MaterialExpressionMultiply, const_b=0.5)
graph.connect(gap, half_gap, "A")
from_copy = graph.op(unreal.MaterialExpressionSubtract, A=distance, B=half_gap)
to_copy = graph.op(unreal.MaterialExpressionAbs, from_copy)
graph.output("Distance", 0, "Distance to the nearest copy, in the units of Distance.", to_copy)
graph.finish()

# --- MF_Stroke_Hard: 1 within half a width of the path ---
graph = open_function(FOLDER, "MF_Stroke_Hard",
                      "Hard-edged line of a given width, drawn along the path a distance measures to.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path, such as MF_ParallelLinesDistance's.")
width = graph.input("Width", "Scalar", 1, "Full width of the line, in the units of Distance.")
half_width = graph.node(unreal.MaterialExpressionMultiply, const_b=0.5)
graph.connect(width, half_width, "A")
# Step(Y, X) is 1 where X >= Y.
mask = graph.op(unreal.MaterialExpressionStep, Y=distance, X=half_width)
graph.output("Mask", 0, "1 on the line, 0 off it.", mask)
graph.finish()

# --- MF_Stroke_Linear: 1 on the path, 0.5 at half a width, 0 at a width ---
graph = open_function(FOLDER, "MF_Stroke_Linear",
                      "Soft line drawn along the path a distance measures to, fading linearly away from it. At half "
                      "brightness it is as wide as MF_Stroke_Hard, so the two swap.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path, such as MF_CircleDistance's.")
width = graph.input("Width", "Scalar", 1, "Width of the line at half brightness, in the units of Distance. It fades "
                    "out completely this far from the path.")
in_widths = graph.op(unreal.MaterialExpressionDivide, A=distance, B=width)
fade = graph.op(unreal.MaterialExpressionOneMinus, in_widths)
mask = graph.op(unreal.MaterialExpressionSaturate, fade)
graph.output("Mask", 0, "1 on the path, fading to 0 at Width away.", mask)
graph.finish()

# --- MF_Stroke_Smooth: 1 - smoothstep(0, Width, Distance) ---
graph = open_function(FOLDER, "MF_Stroke_Smooth",
                      "Soft line drawn along the path a distance measures to, fading away from it with rounded "
                      "shoulders. At half brightness it is as wide as MF_Stroke_Hard, so the strokes swap.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path, such as MF_CircleDistance's.")
width = graph.input("Width", "Scalar", 1, "Width of the line at half brightness, in the units of Distance. It fades "
                    "out completely this far from the path.")
fade = graph.node(unreal.MaterialExpressionSmoothStep, const_min=0.0)
graph.connect(width, fade, "Max")
graph.connect(distance, fade, "Value")
mask = graph.op(unreal.MaterialExpressionOneMinus, fade)
graph.output("Mask", 0, "1 on the path, easing to 0 at Width away.", mask)
graph.finish()
stroke_smooth = graph.asset

# --- MF_TwoToneStroke: two smooth strokes of one distance, a wide glow and a narrow core ---
graph = open_function(FOLDER, "MF_TwoToneStroke",
                      "A soft line drawn twice over one distance: a wide Glow and a narrow Core inside it, both scaled "
                      "by Intensity. MF_TwoToneColor colours the pair.", CATEGORY)
distance = graph.input("Distance", "Scalar", 0, "Distance to the path.")
width = graph.input("Width", "Scalar", 1, "Width of the glow at half brightness, in the units of Distance.")
core_width = graph.input("CoreWidth", "Scalar", 2, "Width of the core at half brightness, in the units of Distance. "
                         "Narrower than Width.")
intensity = graph.input("Intensity", "Scalar", 3, "Scales both, 0 to 1.")
glow_line = graph.call(stroke_smooth, Distance=distance, Width=width)
core_line = graph.call(stroke_smooth, Distance=distance, Width=core_width)
glow = graph.op(unreal.MaterialExpressionMultiply, A=(glow_line, "Mask"), B=intensity)
core = graph.op(unreal.MaterialExpressionMultiply, A=(core_line, "Mask"), B=intensity)
graph.output("Glow", 0, "Brightness of the line, 0 to 1.", glow)
graph.output("Core", 1, "The part of Glow in the core, 0 to Glow.", core)
graph.finish()

# --- MF_TwoToneColor: EdgeColor * (Glow - Core) + CoreColor * Core, times Brightness ---
graph = open_function(FOLDER, "MF_TwoToneColor",
                      "Colours a glow in two tones: its Core share in CoreColor, the rest in EdgeColor, all times "
                      "Brightness.", CATEGORY)
glow = graph.input("Glow", "Scalar", 0, "Brightness, 0 to 1.")
core = graph.input("Core", "Scalar", 1, "The part of Glow drawn in CoreColor. Anything above Glow counts as Glow.")
core_color = graph.input("CoreColor", "Vector3", 2, "Colour of the core share.")
edge_color = graph.input("EdgeColor", "Vector3", 3, "Colour of the rest of the glow.")
brightness = graph.input("Brightness", "Scalar", 4, "Multiplies the result.")
core_share = graph.op(unreal.MaterialExpressionMin, A=core, B=glow)
edge_share = graph.op(unreal.MaterialExpressionSubtract, A=glow, B=core_share)
edge_light = graph.op(unreal.MaterialExpressionMultiply, A=edge_color, B=edge_share)
core_light = graph.op(unreal.MaterialExpressionMultiply, A=core_color, B=core_share)
light = graph.op(unreal.MaterialExpressionAdd, A=core_light, B=edge_light)
color = graph.op(unreal.MaterialExpressionMultiply, A=light, B=brightness)
graph.output("Color", 0, "Linear colour, for emissive.", color)
graph.finish()

# --- MF_SharedBit: frac(x / Period) >= 0.5 and frac(y / Period) >= 0.5 ---
graph = open_function(FOLDER, "MF_SharedBit",
                      "1 where the whole parts of both coordinates have one binary digit set: the digit worth half of "
                      "Period, a power of two.", CATEGORY)
coordinates = graph.input("Coordinates", "Vector2", 0, "The two values to test.")
period = graph.input("Period", "Scalar", 1, "Twice the digit's worth: 2 tests the ones, 4 the twos, and so on.")
in_periods = graph.op(unreal.MaterialExpressionDivide, A=coordinates, B=period)
wrapped = graph.op(unreal.MaterialExpressionFrac, in_periods)
# Step(Y, X) is 1 where X >= Y: the upper half of every period has the digit set.
digits = graph.node(unreal.MaterialExpressionStep, const_y=0.5)
graph.connect(wrapped, digits, "X")
first = graph.mask(digits, "r")
second = graph.mask(digits, "g")
both = graph.op(unreal.MaterialExpressionMultiply, A=first, B=second)
graph.output("Mask", 0, "1 where both have the digit, 0 elsewhere.", both)
graph.finish()
shared_bit = graph.asset

# --- MF_SierpinskiMask: no binary digit shared over five digits ---
graph = open_function(FOLDER, "MF_SierpinskiMask",
                      "1 on the Sierpinski triangle: where the whole parts of the two coordinates share no binary "
                      "digit, which is Pascal's triangle taken modulo 2. Five digits are tested, so the pattern "
                      "repeats every 32 along both coordinates.", CATEGORY)
coordinates = graph.input("Coordinates", "Vector2", 0, "Position along the two sides the triangle spans, in cells.")
digits = []
for digit in range(5):
    period = graph.node(unreal.MaterialExpressionConstant, r=2.0 ** (digit + 1))
    digits.append((graph.call(shared_bit, Coordinates=coordinates, Period=period), "Mask"))

mask = graph.op(unreal.MaterialExpressionOneMinus, graph.fold(unreal.MaterialExpressionMax, digits))
graph.output("Mask", 0, "1 on the triangle, 0 in its holes.", mask)
graph.finish()

# --- MF_LissajousPoint: Center + Spread * sin(Frequency * Time + Phase), per axis ---
graph = open_function(FOLDER, "MF_LissajousPoint",
                      "A point travelling a Lissajous figure around a centre: each axis swings back and forth on its "
                      "own frequency, so unrelated frequencies trace a path that takes long to repeat.", CATEGORY)
center = graph.input("Center", "Vector2", 0, "Middle of the figure.")
spread = graph.input("Spread", "Scalar", 1, "Furthest the point goes from Center along each axis, in the units of "
                     "Center.")
time = graph.input("Time", "Scalar", 2, "Seconds, or any clock.")
frequency = graph.input("Frequency", "Vector2", 3, "Swings per second along X and along Y.")
phase = graph.input("Phase", "Vector2", 4, "Start of each swing, in turns.")
swings = graph.op(unreal.MaterialExpressionMultiply, A=frequency, B=time)
shifted = graph.op(unreal.MaterialExpressionAdd, A=swings, B=phase)
# Sine's default period of 1 reads its input in turns.
sway = graph.op(unreal.MaterialExpressionSine, shifted)
offset = graph.op(unreal.MaterialExpressionMultiply, A=sway, B=spread)
point = graph.op(unreal.MaterialExpressionAdd, A=center, B=offset)
graph.output("Point", 0, "Where the point is now, in the units of Center.", point)
graph.finish()

# --- MF_RandomFromPosition: frac(sin(dot(Position, (12.9898, 78.233))) * 43758.5453) ---
graph = open_function(FOLDER, "MF_RandomFromPosition",
                      "A repeatable pseudo-random value from 0 to 1 for a position: the same position always gives "
                      "the same value, neighbouring ones unrelated values. Keep the position within a few thousand "
                      "units, such as cells rather than cm, or the values lose their spread.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Position to draw the value for, such as a cell's centre in cells.")
key = graph.node(unreal.MaterialExpressionConstant2Vector, r=12.9898, g=78.233)
seed = graph.op(unreal.MaterialExpressionDotProduct, A=position, B=key)
# Sine reads its input in turns; the period changes nothing for a hash.
wave = graph.op(unreal.MaterialExpressionSine, seed)
spread = graph.node(unreal.MaterialExpressionMultiply, const_b=43758.5453)
graph.connect(wave, spread, "A")
value = graph.op(unreal.MaterialExpressionFrac, spread)
graph.output("Value", 0, "0 to 1.", value)
graph.finish()

# --- MF_ZeroOneToSigned: Value * 2 - 1 ---
graph = open_function(FOLDER, "MF_ZeroOneToSigned",
                      "A 0 to 1 coordinate remapped to -1 to 1, such as a quad's UV centred on its middle.", CATEGORY)
value = graph.input("Value", "Vector2", 0, "Coordinate, 0 to 1 on each axis.")
centred = graph.node(unreal.MaterialExpressionSubtract, const_b=0.5)
graph.connect(value, centred, "A")
signed = graph.node(unreal.MaterialExpressionMultiply, const_b=2.0)
graph.connect(centred, signed, "A")
graph.output("Signed", 0, "-1 to 1 on each axis, 0 at the middle.", signed)
graph.finish()
zero_one_to_signed = graph.asset

# --- MF_IsWholeNumber: 1 - saturate(|Value - Whole|) ---
graph = open_function(FOLDER, "MF_IsWholeNumber",
                      "1 where a whole-number value is the given whole number, else 0: reads a mode stored as a "
                      "scalar.", CATEGORY)
value = graph.input("Value", "Scalar", 0, "A whole number, such as a mode.")
whole = graph.input("Whole", "Scalar", 1, "The whole number to test for.")
away = graph.op(unreal.MaterialExpressionSubtract, A=value, B=whole)
distance = graph.op(unreal.MaterialExpressionAbs, away)
near = graph.op(unreal.MaterialExpressionSaturate, distance)
is_whole = graph.op(unreal.MaterialExpressionOneMinus, near)
graph.output("Is", 0, "1 when Value is Whole, 0 for any other whole number.", is_whole)
graph.finish()

# --- MF_DurationWipe: Step(frac(atan2(y, x) / 2pi + 0.25), 1 - Spent) over the quad centred to [-1, 1] ---
graph = open_function(FOLDER, "MF_DurationWipe",
                      "Clock-wipe mask for a remaining-life readout: the wedge of a disc still alive once Spent of it "
                      "is used up, emptying clockwise from 12 o'clock.", CATEGORY)
uv = graph.input("UV", "Vector2", 0, "Texture coordinates of the disc's quad.")
spent = graph.input("Spent", "Scalar", 1, "Fraction used up, 0 to 1. At 0 the disc is full, so an unwritten custom "
                    "primitive data slot reads as untouched.")
quad = graph.call(zero_one_to_signed, Value=uv)
# UV y grows downward, which makes the angle run clockwise on screen.
polar = graph.call(polar_coordinates, Position=(quad, "Signed"))
from_noon = graph.node(unreal.MaterialExpressionAdd, const_b=0.25)
graph.connect((polar, "Angle"), from_noon, "A")
# atan2 spans half a turn either side of 0; frac wraps the negative arc onto the top of the range.
turn = graph.op(unreal.MaterialExpressionFrac, from_noon)
alive = graph.op(unreal.MaterialExpressionOneMinus, spent)
# Step(Y, X) is 1 where X >= Y.
wipe = graph.op(unreal.MaterialExpressionStep, Y=turn, X=alive)
graph.output("Wipe", 0, "1 on the wedge still alive, 0 on the spent part.", wipe)
graph.finish()

unreal.log(f"GENERICFUNCTIONS::built {FOLDER}")
