"""Colour patterns: functions splitting a surface into big geometric regions, one colour per gameplay meaning.

Materials call MF_MeaningColors, never a pattern directly: PATTERN is the one place choosing what every effect draws,
and MPC_MeaningColors the one size and speed it is drawn at, in world space, so every effect shows the same bands
whatever its shape or scale. Telegraphs draw their own pattern, stripes, through MF_TelegraphColors
(make_beam_and_telegraph_materials.py), at the collection's stripe width and angle. Every MF_ColorPattern_* shares one
pin contract (Position, Scroll, ColorCount in; ColorIndex out)."""
import unreal

FOLDER = "/Game/Art/VFX/Generic/Materials/Functions"
COLLECTION_FOLDER = "/Game/Art/VFX/Generic/Materials"
# Each created with this value only: a rebuild keeps whatever the collection has been tuned to since.
COLLECTION_KNOBS = {
    "PatternSize": 100.0,  # world cm per band and per tooth
    "PatternSpeed": 0.5,  # bands per second, toward world +Y
    "TelegraphStripeWidth": 50.0,  # world cm per stripe
    "TelegraphStripeAngle": -45.0,  # degrees from world X of the direction the stripes are stacked along
}
CATEGORY = "GeoTrinity|Pattern"
PATTERN = "MF_ColorPattern_Zigzag"  # drawn by every MF_MeaningColors caller
MAX_COLORS = 4
ZIGZAG_DEPTH = 0.4  # how far a tooth reaches either side of its band's middle, in bands
ZIGZAG_FLATNESS = 2.0  # slope steepness: 2 leaves half of every tooth period flat
QUAD_WIDTH = 0.8  # in columns
QUAD_LENGTH = 1.2  # in cells, out of a repeat 2 long

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
open_function = toolkit["open_function"]
repeat_distance = toolkit["load"](FOLDER + "/MF_RepeatDistance")
random_from_position = toolkit["load"](FOLDER + "/MF_RandomFromPosition")

# --- MF_WrapIndex: Value - Count * floor((Value + 0.5) / Count) ---
graph = open_function(FOLDER, "MF_WrapIndex",
                      "A whole number wrapped into 0 to Count - 1, negatives included: which of Count colours a "
                      "numbered band or overlap takes.", CATEGORY)
value = graph.input("Value", "Scalar", 0, "Whole number to wrap, any sign.")
count = graph.input("Count", "Scalar", 1, "How many values to cycle through, 1 or more.")
# The half keeps the division off whole numbers, where float error would floor the wrong way.
centred = graph.node(unreal.MaterialExpressionAdd, const_b=0.5)
graph.connect(value, centred, "A")
in_cycles = graph.op(unreal.MaterialExpressionDivide, A=centred, B=count)
whole_cycles = graph.op(unreal.MaterialExpressionFloor, in_cycles)
cycled = graph.op(unreal.MaterialExpressionMultiply, A=whole_cycles, B=count)
index = graph.op(unreal.MaterialExpressionSubtract, A=value, B=cycled)
graph.output("Index", 0, "0 to Count - 1.", index)
graph.finish()
wrap_index = graph.asset

# --- MF_PickColor: Color0, replaced by each ColorN once Index reaches it ---
graph = open_function(FOLDER, "MF_PickColor", f"One of {MAX_COLORS} colours, picked by index.", CATEGORY)
index = graph.input("Index", "Scalar", 0, f"Which colour, 0 to {MAX_COLORS - 1}, such as a pattern's ColorIndex.")
color = graph.input("Color0", "Vector3", 1, "Colour at index 0.")
for slot in range(1, MAX_COLORS):
    slot_color = graph.input(f"Color{slot}", "Vector3", slot + 1, f"Colour at index {slot}.")
    # Step(Y, X) is 1 where X >= Y: from this slot's index up.
    reached = graph.node(unreal.MaterialExpressionStep, const_y=slot - 0.5)
    graph.connect(index, reached, "X")
    color = graph.op(unreal.MaterialExpressionLinearInterpolate, A=color, B=slot_color, Alpha=reached)

graph.output("Color", 0, "The picked colour.", color)
graph.finish()
pick_color = graph.asset

# --- MF_SlidingQuads: one quad per column, repeating every 2 along it, each column shifted at random ---
graph = open_function(FOLDER, "MF_SlidingQuads",
                      f"A column of quads on every whole X, {QUAD_WIDTH} wide and {QUAD_LENGTH} long, repeating every "
                      f"2 along Y; each column starts at its own random height and all of them slide by Scroll.",
                      CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to test, in columns along X and cells along Y.")
scroll = graph.input("Scroll", "Scalar", 1, "How far every quad has slid along +Y, in cells.")
seed = graph.input("Seed", "Scalar", 2, "Picks another set of random column heights.")
position_x = graph.mask(position, "r")
position_y = graph.mask(position, "g")
one =graph.node(unreal.MaterialExpressionConstant, r=1.0)
across = graph.call(repeat_distance, Value=position_x, Spacing=one)
column = graph.op(unreal.MaterialExpressionRound, position_x)
key = graph.op(unreal.MaterialExpressionAppendVector, A=column, B=seed)
height = graph.call(random_from_position, Position=key)
shifted = graph.op(unreal.MaterialExpressionAdd, A=position_y, B=(height, "Value"))
# Y minus Scroll moves every quad toward +Y as Scroll grows.
slid = graph.op(unreal.MaterialExpressionSubtract, A=shifted, B=scroll)
two = graph.node(unreal.MaterialExpressionConstant, r=2.0)
along = graph.call(repeat_distance, Value=slid, Spacing=two)
# Step(Y, X) is 1 where X >= Y.
inside_across = graph.node(unreal.MaterialExpressionStep, const_x=QUAD_WIDTH / 2)
graph.connect((across, "Distance"), inside_across, "Y")
inside_along = graph.node(unreal.MaterialExpressionStep, const_x=QUAD_LENGTH / 2)
graph.connect((along, "Distance"), inside_along, "Y")
mask = graph.op(unreal.MaterialExpressionMultiply, A=inside_across, B=inside_along)
graph.output("Mask", 0, "1 inside a quad, 0 outside.", mask)
graph.finish()
sliding_quads = graph.asset

# --- MF_ColorPattern_Zigzag: bands along Y whose edges zigzag along X with flat tops, numbered and wrapped ---
graph = open_function(FOLDER, "MF_ColorPattern_Zigzag",
                      "Bands one unit wide stacked along Y, their edges zigzagging along X one tooth per unit, tops "
                      "and bottoms flat. Neighbouring bands take the next colour, cycling through ColorCount, so "
                      "every colour covers the same share.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to colour: X runs along the teeth, one per unit, Y across the "
                       "bands, one per unit.")
scroll = graph.input("Scroll", "Scalar", 1, "How far the bands have travelled toward +Y, in bands.")
color_count = graph.input("ColorCount", "Scalar", 2, f"How many colours to cycle through, 1 to {MAX_COLORS}.")
position_x = graph.mask(position, "r")
position_y = graph.mask(position, "g")
one = graph.node(unreal.MaterialExpressionConstant, r=1.0)
tooth = graph.call(repeat_distance, Value=position_x, Spacing=one)
# Distance to the nearest whole X runs 0 to 0.5: stretched to -1..1, steepened, then cut flat at both ends.
wave = graph.node(unreal.MaterialExpressionMultiply, const_b=4.0)
graph.connect((tooth, "Distance"), wave, "A")
centred_wave = graph.node(unreal.MaterialExpressionSubtract, const_b=1.0)
graph.connect(wave, centred_wave, "A")
steep_wave = graph.node(unreal.MaterialExpressionMultiply, const_b=ZIGZAG_FLATNESS)
graph.connect(centred_wave, steep_wave, "A")
flat_wave = graph.node(unreal.MaterialExpressionClamp, min_default=-1.0, max_default=1.0)
graph.connect(steep_wave, flat_wave)
tooth_offset = graph.node(unreal.MaterialExpressionMultiply, const_b=ZIGZAG_DEPTH)
graph.connect(flat_wave, tooth_offset, "A")
travelled = graph.op(unreal.MaterialExpressionSubtract, A=position_y, B=scroll)
toothed = graph.op(unreal.MaterialExpressionAdd, A=travelled, B=tooth_offset)
band = graph.op(unreal.MaterialExpressionFloor, toothed)
wrapped = graph.call(wrap_index, Value=band, Count=color_count)
graph.output("ColorIndex", 0, "Which colour the point takes, 0 to ColorCount - 1.", (wrapped, "Index"))
graph.finish()

# --- MF_ColorPattern_Overlap: two sets of sliding quads, coloured by how many cover the point ---
graph = open_function(FOLDER, "MF_ColorPattern_Overlap",
                      "Two sets of quad columns half a column apart, sliding opposite ways along Y. A point's colour "
                      "is how many quads cover it, 0 to 2, wrapped into ColorCount: with two colours every overlap "
                      "flips the colour back. Shares stay even only with two colours.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to colour: X runs across the columns, one per unit, Y along "
                       "them.")
scroll = graph.input("Scroll", "Scalar", 1, "How far the first set has slid toward +Y and the second toward -Y.")
color_count = graph.input("ColorCount", "Scalar", 2, f"How many colours to cycle through, 1 to {MAX_COLORS}.")
first_seed = graph.node(unreal.MaterialExpressionConstant, r=0.0)
first = graph.call(sliding_quads, Position=position, Scroll=scroll, Seed=first_seed)
half_column = graph.node(unreal.MaterialExpressionConstant2Vector, r=0.5, g=0.0)
second_position = graph.op(unreal.MaterialExpressionAdd, A=position, B=half_column)
backward = graph.node(unreal.MaterialExpressionMultiply, const_b=-1.0)
graph.connect(scroll, backward, "A")
second_seed = graph.node(unreal.MaterialExpressionConstant, r=1.0)
second = graph.call(sliding_quads, Position=second_position, Scroll=backward, Seed=second_seed)
covering = graph.op(unreal.MaterialExpressionAdd, A=(first, "Mask"), B=(second, "Mask"))
wrapped = graph.call(wrap_index, Value=covering, Count=color_count)
graph.output("ColorIndex", 0, "Which colour the point takes, 0 to ColorCount - 1.", (wrapped, "Index"))
graph.finish()

# --- MF_ColorPattern_Stripes: straight bands along X, numbered and wrapped ---
graph = open_function(FOLDER, "MF_ColorPattern_Stripes",
                      "Straight bands one unit wide stacked along X, neighbouring bands taking the next colour, "
                      "cycling through ColorCount, so every colour covers the same share.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Point to colour: the bands are stacked along X, one per unit; Y is "
                       "ignored.")
scroll = graph.input("Scroll", "Scalar", 1, "How far the bands have travelled toward +X, in bands.")
color_count = graph.input("ColorCount", "Scalar", 2, f"How many colours to cycle through, 1 to {MAX_COLORS}.")
travelled = graph.op(unreal.MaterialExpressionSubtract, A=graph.mask(position, "r"), B=scroll)
band = graph.op(unreal.MaterialExpressionFloor, travelled)
wrapped = graph.call(wrap_index, Value=band, Count=color_count)
graph.output("ColorIndex", 0, "Which colour the point takes, 0 to ColorCount - 1.", (wrapped, "Index"))
graph.finish()

# --- MPC_MeaningColors: the sizes, speed and angle every meaning-colour pattern draws at ---
collection = toolkit["load_or_create"](COLLECTION_FOLDER, "MPC_MeaningColors", unreal.MaterialParameterCollection,
                                       unreal.MaterialParameterCollectionFactoryNew())
scalars = list(collection.get_editor_property("scalar_parameters"))
existing = [str(scalar.get_editor_property("parameter_name")) for scalar in scalars]
missing = [name for name in COLLECTION_KNOBS if name not in existing]
for name in missing:
    scalar = unreal.CollectionScalarParameter()
    scalar.set_editor_property("parameter_name", name)
    scalar.set_editor_property("default_value", COLLECTION_KNOBS[name])
    scalars.append(scalar)

if missing:
    # Each parameter gets its id in the post-change notification.
    collection.set_editor_property("scalar_parameters", scalars, toolkit["ALWAYS"])
    assert set(COLLECTION_KNOBS) <= {str(name) for name in collection.get_scalar_parameter_names()}, \
        "MPC_MeaningColors: parameters would not register"
    toolkit["save"](collection)

# --- MF_MeaningColors: PATTERN laid over the world, its colour index turned into its colour ---
# The one generic function reading world position and a collection: every effect must draw the same bands.
graph = open_function(FOLDER, "MF_MeaningColors",
                      f"Up to {MAX_COLORS} colours, one per gameplay meaning, split into big hard-edged regions by the "
                      f"project's colour pattern ({PATTERN}), laid over the world at MPC_MeaningColors' size and "
                      f"speed. Every multi-colour effect calls this, so the pattern is the same on every shape and "
                      f"changes in one place.", CATEGORY)
color_count = graph.input("ColorCount", "Scalar", 0, f"How many colours to show, 1 to {MAX_COLORS}: 1 is a plain "
                          "Color0.")
colors = {f"Color{slot}": graph.input(f"Color{slot}", "Vector3", 1 + slot, f"Colour of meaning {slot + 1}.")
          for slot in range(MAX_COLORS)}
world = graph.mask(graph.node(unreal.MaterialExpressionWorldPosition), "rg")
size = graph.collection_parameter(collection, "PatternSize")
position = graph.op(unreal.MaterialExpressionDivide, A=world, B=size)
speed = graph.collection_parameter(collection, "PatternSpeed")
time = graph.node(unreal.MaterialExpressionTime)
scroll = graph.op(unreal.MaterialExpressionMultiply, A=time, B=speed)
pattern = graph.call(toolkit["load"](f"{FOLDER}/{PATTERN}"), Position=position, Scroll=scroll,
                     ColorCount=color_count)
picked = graph.call(pick_color, Index=(pattern, "ColorIndex"), **colors)
graph.output("Color", 0, "The colour of the point's region.", (picked, "Color"))
graph.finish()

# --- MF_ColorScale: brightest channel of Scaled over brightest channel of Original ---
graph = open_function(FOLDER, "MF_ColorScale", "How many times brighter a colour is than the one it was scaled from, "
                      "such as a particle colour faded over its life against the colour it started as.", CATEGORY)
scaled = graph.input("Scaled", "Vector3", 0, "The scaled colour.")
original = graph.input("Original", "Vector3", 1, "The colour before scaling.")


def brightest_channel(color):
    red_or_green = graph.op(unreal.MaterialExpressionMax, A=graph.mask(color, "r"),
                            B=graph.mask(color, "g"))
    return graph.op(unreal.MaterialExpressionMax, A=red_or_green, B=graph.mask(color, "b"))


# The floor keeps a black original from dividing by zero.
original_brightness = graph.node(unreal.MaterialExpressionMax, const_b=0.0001)
graph.connect(brightest_channel(original), original_brightness, "A")
scale = graph.op(unreal.MaterialExpressionDivide, A=brightest_channel(scaled), B=original_brightness)
graph.output("Scale", 0, "Scaled's brightest channel over Original's: 1 while unscaled.", scale)
graph.finish()

unreal.log(f"COLORPATTERNS::built {FOLDER} drawing {PATTERN}")
