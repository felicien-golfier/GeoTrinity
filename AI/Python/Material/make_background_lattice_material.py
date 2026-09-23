"""Floor triangle line art: pattern and ring layers, the glow blend, the material, its instance, the pulse collection.

The material is one layer stack. Layer 0 draws the lines, every layer above it adds a glow through MLB_AddGlow, and
MI_BackgroundLattice swaps, adds or hides them. make_background_looks.py builds the other glow layers.
Needs the generic functions from make_generic_material_functions.py. Run outside PIE.
"""
import unreal

FOLDER = "/Game/Art/VFX/Background"
FUNCTIONS = f"{FOLDER}/Functions"
LAYERS = f"{FOLDER}/Layers"
GENERIC = "/Game/Art/VFX/Generic/Materials/Functions"
CATEGORY = "GeoTrinity|Background"
SLOT_COUNT = 8
SLOT_FORMAT = "PulseSource_{:02d}"
SQRT3_OVER_2 = 0.8660254037844386
ONE_OVER_SQRT3 = 0.5773502691896258
EDGE_NORMALS = ((0.0, 1.0), (-SQRT3_OVER_2, 0.5), (SQRT3_OVER_2, 0.5))
# Fixed, so an instance's own layer stack stays linked to this one across rebuilds.
GLOW_LAYER_GUID = "6A3F2C1E8B4D4E9FA1C27D5B3E8F0A11"

SHAPE_SIZE = 200.0
LINE_THICKNESS = 8.0
LINE_GAP = 20.0
LINE_COLOR = unreal.LinearColor(0.001, 0.001, 0.001, 1.0)
INNER_SIZE = 60.0
INNER_SCALE = 1.0
INNER_TURN_SPEED = 0.02
INNER_ROTATION = 0.0
RING_WIDTH = 100.0
PULSE_COLOR = unreal.LinearColor(0.114583, 0.050431, 0.094441, 1.0)
PULSE_BRIGHTNESS = 3.0

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
open_function = toolkit["open_function"]
open_layer = toolkit["open_layer"]
load = toolkit["load"]
save = toolkit["save"]
scalar = unreal.MaterialExpressionScalarParameter
vector = unreal.MaterialExpressionVectorParameter

parallel_lines_distance = load(f"{GENERIC}/MF_ParallelLinesDistance")
circle_distance = load(f"{GENERIC}/MF_CircleDistance")
polygon_distance = load(f"{GENERIC}/MF_PolygonDistance")
triangle_cell = load(f"{GENERIC}/MF_TriangleCell")
double_line_distance = load(f"{GENERIC}/MF_DoubleLineDistance")
stroke_hard = load(f"{GENERIC}/MF_Stroke_Hard")
stroke_linear = load(f"{GENERIC}/MF_Stroke_Linear")

# --- MPC_BackgroundPulse: (OriginX, OriginY, Radius, Intensity) per slot, written by BP_GeoCam's BackgroundPulse ---
collection = toolkit["load_or_create"](FOLDER, "MPC_BackgroundPulse", unreal.MaterialParameterCollection,
                                       unreal.MaterialParameterCollectionFactoryNew())
slot_names = [SLOT_FORMAT.format(index) for index in range(SLOT_COUNT)]
if [str(name) for name in collection.get_vector_parameter_names()] != slot_names:
    slots = []
    for name in slot_names:
        slot = unreal.CollectionVectorParameter()
        slot.set_editor_property("parameter_name", name)
        slot.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
        slots.append(slot)

    # Each slot gets its id in the post-change notification.
    collection.set_editor_property("vector_parameters", slots, toolkit["ALWAYS"])
    assert [str(name) for name in collection.get_vector_parameter_names()] == slot_names, \
        "MPC_BackgroundPulse: slots would not register"
    save(collection)

# --- MF_Pattern_TriangleLattice: doubled triangle edges, and a small turning triangle inside each cell ---
graph = open_function(FUNCTIONS, "MF_Pattern_TriangleLattice",
                      "Pattern of ML_Pattern_TriangleLattice: equilateral triangles drawn with doubled edges, a small "
                      "triangle inside each one. Every MF_Pattern_* takes Position, ShapeSize and LineThickness and "
                      "gives LineMask, so one replaces another in the call's Material Function field; its other pins "
                      "are its own.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1560, -300)
shape_size = graph.input("ShapeSize", "Scalar", 1, "Side length of one triangle, in cm.", -1560, 240)
line_thickness = graph.input("LineThickness", "Scalar", 2, "Full width of every line, in cm.", -1560, 400)
line_gap = graph.input("LineGap", "Scalar", 3, "Distance between the two lines of an edge, centre to centre, in cm. "
                       "0 draws one line.", -1560, 520)
inner_size = graph.input("InnerSize", "Scalar", 4, "Side length of the inner triangle, in cm.", -1560, 760)
inner_rotation = graph.input("InnerRotation", "Scalar", 5, "Turn of the inner triangle, in turns.", -1560, 880)
inner_scale = graph.input("InnerScale", "Scalar", 6, "Size multiplier of the inner triangle.", -1560, 1000)

# Three sets of parallel lines 60 degrees apart, all through the origin, cut the plane into the triangles.
# A triangle's height is the gap between two parallel edges.
spacing = graph.node(unreal.MaterialExpressionMultiply, -1300, 240, const_b=SQRT3_OVER_2)
graph.connect(shape_size, spacing, "A")
distances = []
for index, (normal_x, normal_y) in enumerate(EDGE_NORMALS):
    row = -360 + index * 200
    normal = graph.node(unreal.MaterialExpressionConstant2Vector, -1300, row + 40, r=normal_x, g=normal_y)
    lines = graph.call(parallel_lines_distance, -1020, row, Position=position, Normal=normal, Spacing=spacing)
    distances.append((lines, "Distance"))

nearest = graph.op(unreal.MaterialExpressionMin, -680, -260, A=distances[0], B=distances[1])
nearest = graph.op(unreal.MaterialExpressionMin, -540, -80, A=nearest, B=distances[2])
doubled = graph.call(double_line_distance, -380, 0, Distance=nearest, Gap=line_gap)
edges = graph.call(stroke_hard, -80, 80, Distance=(doubled, "Distance"), Width=line_thickness)

cell = graph.call(triangle_cell, -1020, 620, Position=position, Size=shape_size)
scaled_size = graph.op(unreal.MaterialExpressionMultiply, -1300, 840, A=inner_size, B=inner_scale)
corner_radius = graph.node(unreal.MaterialExpressionMultiply, -1140, 840, const_b=ONE_OVER_SQRT3)
graph.connect(scaled_size, corner_radius, "A")
sides = graph.node(unreal.MaterialExpressionConstant, -1020, 780, r=3.0)
inner = graph.call(polygon_distance, -720, 680, Position=(cell, "CellPosition"), Sides=sides, Radius=corner_radius,
                   Rotation=inner_rotation)
inner_line = graph.call(stroke_hard, -380, 720, Distance=(inner, "Distance"), Width=line_thickness)

line_mask = graph.op(unreal.MaterialExpressionMax, 160, 360, A=(edges, "Mask"), B=(inner_line, "Mask"))
graph.output("LineMask", 0, "1 on the lines, 0 elsewhere.", line_mask, 320, 360)
graph.finish()
triangle_lattice = graph.asset

# --- MF_PulseRing: one slot's ring ---
graph = open_function(FUNCTIONS, "MF_PulseRing",
                      "One ring of the background wave, drawn from a PulseSource slot of MPC_BackgroundPulse.",
                      CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1000, -120)
source = graph.input("PulseSource", "Vector4", 1, "(OriginX, OriginY, Radius, Intensity), in cm. An all-zero slot "
                     "draws nothing.", -1000, 60)
ring_width = graph.input("RingWidth", "Scalar", 2, "Width of the ring at half brightness, in cm.", -1000, 260)
origin = graph.mask(source, "rg", -760, -20)
radius = graph.mask(source, "b", -760, 80)
intensity = graph.mask(source, "a", -760, 180)
from_ring = graph.call(circle_distance, -560, -80, Position=position, Center=origin, Radius=radius)
stroke = graph.call(stroke_linear, -260, 60, Distance=(from_ring, "Distance"), Width=ring_width)
ring = graph.op(unreal.MaterialExpressionMultiply, 0, 140, A=(stroke, "Mask"), B=intensity)
graph.output("Ring", 0, "Intensity on the ring, fading to 0 at RingWidth either side.", ring, 160, 140)
graph.finish()
pulse_ring = graph.asset

# --- MF_Wave_PulseRings: the brightest ring over every slot ---
graph = open_function(FUNCTIONS, "MF_Wave_PulseRings",
                      "Rings of ML_Wave_Rings: the brightest of the rings MPC_BackgroundPulse holds. BP_GeoCam's "
                      "BackgroundPulse component moves and grows them: speed, reach and count live there.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.", -1100, 240)
ring_width = graph.input("RingWidth", "Scalar", 1, "Width of each ring at half brightness, in cm.", -1100, 880)
rings = []
for index, name in enumerate(slot_names):
    row = index * 160
    slot = graph.collection_parameter(collection, name, -820, row + 20)
    ring = graph.call(pulse_ring, -520, row, Position=position, PulseSource=slot, RingWidth=ring_width)
    rings.append((ring, "Ring"))

pulse = rings[0]
for index, ring in enumerate(rings[1:], 1):
    pulse = graph.op(unreal.MaterialExpressionMax, -180, index * 160 + 30, A=pulse, B=ring)

graph.output("Pulse", 0, "0 to 1: how brightly the wave lights each point.", pulse, 20, (SLOT_COUNT - 1) * 160 + 30)
graph.finish()
pulse_rings = graph.asset

# --- ML_Pattern_TriangleLattice: layer 0, the lines themselves ---
graph = open_layer(LAYERS, "ML_Pattern_TriangleLattice",
                   "Pattern layer of M_BackgroundLattice: the triangle lines, lit by the scene. The glow layers add "
                   "on top of it.", unreal.MaterialFunctionMaterialLayer, unreal.MaterialFunctionMaterialLayerFactory())
# World XY, not UV: the floor pieces are scaled and rotated unevenly, world space keeps one seamless sheet.
world_position = graph.node(unreal.MaterialExpressionWorldPosition, -1560, 160)
world_xy = graph.mask(world_position, "rg", -1320, 160)
line_color = graph.parameter(vector, "LineColor", LINE_COLOR, "01 Pattern", 3,
                             "Colour of the lines, lit by the scene. The glow layers add on top.", -620, -80)
shape_size = graph.parameter(scalar, "ShapeSize", SHAPE_SIZE, "01 Pattern", 0, "Side length of one triangle, in cm.",
                             -1240, 300)
line_thickness = graph.parameter(scalar, "LineThickness", LINE_THICKNESS, "01 Pattern", 1,
                                 "Width of every line, in cm.", -1240, 400)
line_gap = graph.parameter(scalar, "LineGap", LINE_GAP, "01 Pattern", 2,
                           "Distance between the two lines of a triangle edge, centre to centre, in cm. 0 draws one "
                           "line.", -1240, 500)
inner_size = graph.parameter(scalar, "InnerSize", INNER_SIZE, "02 Inner Triangle", 0,
                             "Side length of the small triangle inside each cell, in cm.", -1240, 620)
inner_scale = graph.parameter(scalar, "InnerScale", INNER_SCALE, "02 Inner Triangle", 1,
                              "Size multiplier of the inner triangles, 1 being InnerSize.", -1240, 720)
inner_turn_speed = graph.parameter(scalar, "InnerTurnSpeed", INNER_TURN_SPEED, "02 Inner Triangle", 2,
                                   "Spin of the inner triangles, in turns per second, counter-clockwise. A third of "
                                   "a turn looks like none.", -1560, 960)
inner_rotation = graph.parameter(scalar, "InnerRotation", INNER_ROTATION, "02 Inner Triangle", 3,
                                 "Turn added on top of the spin, in turns.", -1320, 1000)
time = graph.node(unreal.MaterialExpressionTime, -1560, 860)
spin = graph.op(unreal.MaterialExpressionMultiply, -1320, 880, A=time, B=inner_turn_speed)
turn = graph.op(unreal.MaterialExpressionAdd, -1100, 900, A=spin, B=inner_rotation)
pattern = graph.call(triangle_lattice, -940, 300, Position=world_xy, ShapeSize=shape_size,
                     LineThickness=line_thickness, LineGap=line_gap, InnerSize=inner_size,
                     InnerRotation=turn, InnerScale=inner_scale)
lines = graph.op(unreal.MaterialExpressionMakeMaterialAttributes, -380, 100, BaseColor=line_color,
                 OpacityMask=(pattern, "LineMask"))
graph.output("Attributes", 0, "Line colour and line mask, no glow.", lines, -120, 100)
graph.finish()
pattern_layer = graph.asset

# --- ML_Wave_Rings: the rings in one colour ---
graph = open_layer(LAYERS, "ML_Wave_Rings",
                   "Glow layer of M_BackgroundLattice: the rings BP_GeoCam's BackgroundPulse component sends across "
                   "the floor, in one colour.", unreal.MaterialFunctionMaterialLayer,
                   unreal.MaterialFunctionMaterialLayerFactory())
world_position = graph.node(unreal.MaterialExpressionWorldPosition, -1240, -80)
world_xy = graph.mask(world_position, "rg", -1000, -80)
ring_width = graph.parameter(scalar, "RingWidth", RING_WIDTH, "Rings", 0,
                             "Width of a ring at half brightness, in cm. Ring speed, reach and count live on "
                             "BP_GeoCam > BackgroundPulse.", -1000, 20)
pulse_color = graph.parameter(vector, "PulseColor", PULSE_COLOR, "Rings", 1, "Glow colour of the rings.", -560, -220)
pulse_brightness = graph.parameter(scalar, "PulseBrightness", PULSE_BRIGHTNESS, "Rings", 2,
                                   "Glow strength, multiplying PulseColor.", -760, 120)
wave = graph.call(pulse_rings, -760, -40, Position=world_xy, RingWidth=ring_width)
glow = graph.op(unreal.MaterialExpressionMultiply, -520, 0, A=(wave, "Pulse"), B=pulse_brightness)
emissive = graph.op(unreal.MaterialExpressionMultiply, -300, -60, A=pulse_color, B=glow)
light = graph.op(unreal.MaterialExpressionMakeMaterialAttributes, -120, -60, EmissiveColor=emissive)
graph.output("Attributes", 0, "The rings' glow alone.", light, 100, -60)
graph.finish()
rings_layer = graph.asset

# --- MLB_AddGlow: the glow of a layer added to everything below it ---
graph = open_layer(LAYERS, "MLB_AddGlow",
                   "Layer blend of M_BackgroundLattice: adds this layer's glow to everything below it and keeps the "
                   "lines of layer 0. Pick it as the blend of every glow layer.",
                   unreal.MaterialFunctionMaterialLayerBlend, unreal.MaterialFunctionMaterialLayerBlendFactory())
# The stack feeds a blend's inputs in sort order: what is below first, the layer second.
bottom = graph.input("Bottom", "MaterialAttributes", 0, "Everything below this layer.", -760, -80)
top = graph.input("Top", "MaterialAttributes", 1, "This layer.", -760, 80)
below = graph.op(unreal.MaterialExpressionBreakMaterialAttributes, -540, -120, Attr=bottom)
layer = graph.op(unreal.MaterialExpressionBreakMaterialAttributes, -540, 160, Attr=top)
glow = graph.op(unreal.MaterialExpressionAdd, -300, 60, A=(below, "EmissiveColor"), B=(layer, "EmissiveColor"))
blended = graph.op(unreal.MaterialExpressionMakeMaterialAttributes, -140, -40, BaseColor=(below, "BaseColor"),
                   OpacityMask=(below, "OpacityMask"), EmissiveColor=glow)
graph.output("Attributes", 0, "The lines below, with both glows added.", blended, 80, -40)
graph.finish()
add_glow = graph.asset

# --- M_BackgroundLattice: one layer stack, rebuilt in place, the Floor mesh's instance is its child ---
graph = toolkit["open_material"](FOLDER, "M_BackgroundLattice")
layer_stack = graph.layer_stack([pattern_layer, rings_layer], [add_glow], ["Pattern", "Glow"], [GLOW_LAYER_GUID],
                                -400, 0)
graph.to_property(layer_stack, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
graph.asset.set_editor_property("use_material_attributes", True)
graph.asset.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
graph.asset.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
graph.finish()

# Left alone once it exists, so its tuned values and its own layers live.
if not unreal.EditorAssetLibrary.does_asset_exist(f"{FOLDER}/MI_BackgroundLattice"):
    instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "MI_BackgroundLattice", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, graph.asset)
    save(instance)

unreal.log(f"BGLATTICE::built {FOLDER}/M_BackgroundLattice")
