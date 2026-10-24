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
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.")
shape_size = graph.input("ShapeSize", "Scalar", 1, "Side length of one triangle, in cm.")
line_thickness = graph.input("LineThickness", "Scalar", 2, "Full width of every line, in cm.")
line_gap = graph.input("LineGap", "Scalar", 3, "Distance between the two lines of an edge, centre to centre, in cm. "
                       "0 draws one line.")
inner_size = graph.input("InnerSize", "Scalar", 4, "Side length of the inner triangle, in cm.")
inner_rotation = graph.input("InnerRotation", "Scalar", 5, "Turn of the inner triangle, in turns.")
inner_scale = graph.input("InnerScale", "Scalar", 6, "Size multiplier of the inner triangle.")

# Three sets of parallel lines 60 degrees apart, all through the origin, cut the plane into the triangles.
# A triangle's height is the gap between two parallel edges.
spacing = graph.node(unreal.MaterialExpressionMultiply, const_b=SQRT3_OVER_2)
graph.connect(shape_size, spacing, "A")
distances = []
for normal_x, normal_y in EDGE_NORMALS:
    normal = graph.node(unreal.MaterialExpressionConstant2Vector, r=normal_x, g=normal_y)
    lines = graph.call(parallel_lines_distance, Position=position, Normal=normal, Spacing=spacing)
    distances.append((lines, "Distance"))

nearest = graph.fold(unreal.MaterialExpressionMin, distances)
doubled = graph.call(double_line_distance, Distance=nearest, Gap=line_gap)
edges = graph.call(stroke_hard, Distance=(doubled, "Distance"), Width=line_thickness)

cell = graph.call(triangle_cell, Position=position, Size=shape_size)
scaled_size = graph.op(unreal.MaterialExpressionMultiply, A=inner_size, B=inner_scale)
corner_radius = graph.node(unreal.MaterialExpressionMultiply, const_b=ONE_OVER_SQRT3)
graph.connect(scaled_size, corner_radius, "A")
sides = graph.node(unreal.MaterialExpressionConstant, r=3.0)
inner = graph.call(polygon_distance, Position=(cell, "CellPosition"), Sides=sides, Radius=corner_radius,
                   Rotation=inner_rotation)
inner_line = graph.call(stroke_hard, Distance=(inner, "Distance"), Width=line_thickness)

line_mask = graph.op(unreal.MaterialExpressionMax, A=(edges, "Mask"), B=(inner_line, "Mask"))
graph.output("LineMask", 0, "1 on the lines, 0 elsewhere.", line_mask)
graph.finish()
triangle_lattice = graph.asset

# --- MF_PulseRing: one slot's ring ---
graph = open_function(FUNCTIONS, "MF_PulseRing",
                      "One ring of the background wave, drawn from a PulseSource slot of MPC_BackgroundPulse.",
                      CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.")
source = graph.input("PulseSource", "Vector4", 1, "(OriginX, OriginY, Radius, Intensity), in cm. An all-zero slot "
                     "draws nothing.")
ring_width = graph.input("RingWidth", "Scalar", 2, "Width of the ring at half brightness, in cm.")
origin = graph.mask(source, "rg")
radius = graph.mask(source, "b")
intensity = graph.mask(source, "a")
from_ring = graph.call(circle_distance, Position=position, Center=origin, Radius=radius)
stroke = graph.call(stroke_linear, Distance=(from_ring, "Distance"), Width=ring_width)
ring = graph.op(unreal.MaterialExpressionMultiply, A=(stroke, "Mask"), B=intensity)
graph.output("Ring", 0, "Intensity on the ring, fading to 0 at RingWidth either side.", ring)
graph.finish()
pulse_ring = graph.asset

# --- MF_Wave_PulseRings: the brightest ring over every slot ---
graph = open_function(FUNCTIONS, "MF_Wave_PulseRings",
                      "Rings of ML_Wave_Rings: the brightest of the rings MPC_BackgroundPulse holds. BP_GeoCam's "
                      "BackgroundPulse component moves and grows them: speed, reach and count live there.", CATEGORY)
position = graph.input("Position", "Vector2", 0, "Where to draw, in cm.")
ring_width = graph.input("RingWidth", "Scalar", 1, "Width of each ring at half brightness, in cm.")
rings = []
for name in slot_names:
    slot = graph.collection_parameter(collection, name)
    ring = graph.call(pulse_ring, Position=position, PulseSource=slot, RingWidth=ring_width)
    rings.append((ring, "Ring"))

graph.output("Pulse", 0, "0 to 1: how brightly the wave lights each point.",
             graph.fold(unreal.MaterialExpressionMax, rings))
graph.finish()
pulse_rings = graph.asset

# --- ML_Pattern_TriangleLattice: layer 0, the lines themselves ---
graph = open_layer(LAYERS, "ML_Pattern_TriangleLattice",
                   "Pattern layer of M_BackgroundLattice: the triangle lines, lit by the scene. The glow layers add "
                   "on top of it.", unreal.MaterialFunctionMaterialLayer, unreal.MaterialFunctionMaterialLayerFactory())
# World XY, not UV: the floor pieces are scaled and rotated unevenly, world space keeps one seamless sheet.
world_position = graph.node(unreal.MaterialExpressionWorldPosition)
world_xy = graph.mask(world_position, "rg")
line_color = graph.parameter(vector, "LineColor", LINE_COLOR, "01 Pattern", 3,
                             "Colour of the lines, lit by the scene. The glow layers add on top.")
shape_size = graph.parameter(scalar, "ShapeSize", SHAPE_SIZE, "01 Pattern", 0, "Side length of one triangle, in cm.")
line_thickness = graph.parameter(scalar, "LineThickness", LINE_THICKNESS, "01 Pattern", 1,
                                 "Width of every line, in cm.")
line_gap = graph.parameter(scalar, "LineGap", LINE_GAP, "01 Pattern", 2,
                           "Distance between the two lines of a triangle edge, centre to centre, in cm. 0 draws one "
                           "line.")
inner_size = graph.parameter(scalar, "InnerSize", INNER_SIZE, "02 Inner Triangle", 0,
                             "Side length of the small triangle inside each cell, in cm.")
inner_scale = graph.parameter(scalar, "InnerScale", INNER_SCALE, "02 Inner Triangle", 1,
                              "Size multiplier of the inner triangles, 1 being InnerSize.")
inner_turn_speed = graph.parameter(scalar, "InnerTurnSpeed", INNER_TURN_SPEED, "02 Inner Triangle", 2,
                                   "Spin of the inner triangles, in turns per second, counter-clockwise. A third of "
                                   "a turn looks like none.")
inner_rotation = graph.parameter(scalar, "InnerRotation", INNER_ROTATION, "02 Inner Triangle", 3,
                                 "Turn added on top of the spin, in turns.")
time = graph.node(unreal.MaterialExpressionTime)
spin = graph.op(unreal.MaterialExpressionMultiply, A=time, B=inner_turn_speed)
turn = graph.op(unreal.MaterialExpressionAdd, A=spin, B=inner_rotation)
pattern = graph.call(triangle_lattice, Position=world_xy, ShapeSize=shape_size,
                     LineThickness=line_thickness, LineGap=line_gap, InnerSize=inner_size,
                     InnerRotation=turn, InnerScale=inner_scale)
lines = graph.op(unreal.MaterialExpressionMakeMaterialAttributes, BaseColor=line_color,
                 OpacityMask=(pattern, "LineMask"))
graph.output("Attributes", 0, "Line colour and line mask, no glow.", lines)
graph.finish()
pattern_layer = graph.asset

# --- ML_Wave_Rings: the rings in one colour ---
graph = open_layer(LAYERS, "ML_Wave_Rings",
                   "Glow layer of M_BackgroundLattice: the rings BP_GeoCam's BackgroundPulse component sends across "
                   "the floor, in one colour.", unreal.MaterialFunctionMaterialLayer,
                   unreal.MaterialFunctionMaterialLayerFactory())
world_position = graph.node(unreal.MaterialExpressionWorldPosition)
world_xy = graph.mask(world_position, "rg")
ring_width = graph.parameter(scalar, "RingWidth", RING_WIDTH, "Rings", 0,
                             "Width of a ring at half brightness, in cm. Ring speed, reach and count live on "
                             "BP_GeoCam > BackgroundPulse.")
pulse_color = graph.parameter(vector, "PulseColor", PULSE_COLOR, "Rings", 1, "Glow colour of the rings.")
pulse_brightness = graph.parameter(scalar, "PulseBrightness", PULSE_BRIGHTNESS, "Rings", 2,
                                   "Glow strength, multiplying PulseColor.")
wave = graph.call(pulse_rings, Position=world_xy, RingWidth=ring_width)
glow = graph.op(unreal.MaterialExpressionMultiply, A=(wave, "Pulse"), B=pulse_brightness)
emissive = graph.op(unreal.MaterialExpressionMultiply, A=pulse_color, B=glow)
light = graph.op(unreal.MaterialExpressionMakeMaterialAttributes, EmissiveColor=emissive)
graph.output("Attributes", 0, "The rings' glow alone.", light)
graph.finish()
rings_layer = graph.asset

# --- MLB_AddGlow: the glow of a layer added to everything below it ---
graph = open_layer(LAYERS, "MLB_AddGlow",
                   "Layer blend of M_BackgroundLattice: adds this layer's glow to everything below it and keeps the "
                   "lines of layer 0. Pick it as the blend of every glow layer.",
                   unreal.MaterialFunctionMaterialLayerBlend, unreal.MaterialFunctionMaterialLayerBlendFactory())
# The stack feeds a blend's inputs in sort order: what is below first, the layer second.
bottom = graph.input("Bottom", "MaterialAttributes", 0, "Everything below this layer.")
top = graph.input("Top", "MaterialAttributes", 1, "This layer.")
below = graph.op(unreal.MaterialExpressionBreakMaterialAttributes, Attr=bottom)
layer = graph.op(unreal.MaterialExpressionBreakMaterialAttributes, Attr=top)
glow = graph.op(unreal.MaterialExpressionAdd, A=(below, "EmissiveColor"), B=(layer, "EmissiveColor"))
blended = graph.op(unreal.MaterialExpressionMakeMaterialAttributes, BaseColor=(below, "BaseColor"),
                   OpacityMask=(below, "OpacityMask"), EmissiveColor=glow)
graph.output("Attributes", 0, "The lines below, with both glows added.", blended)
graph.finish()
add_glow = graph.asset

# --- M_BackgroundLattice: one layer stack, rebuilt in place, the Floor mesh's instance is its child ---
graph = toolkit["open_material"](FOLDER, "M_BackgroundLattice")
layer_stack = graph.layer_stack([pattern_layer, rings_layer], [add_glow], ["Pattern", "Glow"], [GLOW_LAYER_GUID])
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
