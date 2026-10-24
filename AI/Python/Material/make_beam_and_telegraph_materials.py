"""The beam and its windup telegraph, and the round zone telegraph, rebuilt from shared functions: M_PulseBeam,
M_ZoneIndicatorRay and M_ZoneIndicator. Each draws its fill in up to four colours through MF_MeaningColors, whose
pattern lies over the world, the same on every shape. Rebuilt in place, so every instance and emitter keeps them.

MF_SafeSpotCut cuts the safe spots of MPC_MaskedArea (FSafeSpotMask) out of the two beam materials, only where their
SafeSpotCut parameter is 1; the script adds the collection's SafeSpot_ShadowMode and SafeSpot_LaneDirection parameters
when they are missing.

Run make_color_pattern_functions.py and make_pulse_circle_material.py first. The Color, Color2-4, ColorCount and
SafeSpotCut parameters are bound to the systems' User parameters by AI/Python/Niagara/material_user_bindings.py:
renaming one here means renaming it there."""
import unreal

FOLDER = "/Game/Art/VFX/AOE"
FUNCTIONS = FOLDER + "/Functions"
GENERIC = "/Game/Art/VFX/Generic/Materials/Functions"
CATEGORY = "GeoTrinity|Beam"
MASK_COLLECTION = "/Game/Art/VFX/Abilities/Enemies/PillarsAOE/MPC_MaskedArea"
MASK_SLOTS = 8
SPENT_BRIGHTNESS = 0.2  # of the fill, on the part of the beam's life not yet wiped
COLOR_DEFAULTS = [(0.15, 0.6, 1.0), (1.0, 0.15, 0.1), (0.2, 1.0, 0.3), (1.0, 0.85, 0.1)]

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
load = toolkit["load"]
open_function = toolkit["open_function"]
zero_one_to_signed = load(GENERIC + "/MF_ZeroOneToSigned")
is_whole_number = load(GENERIC + "/MF_IsWholeNumber")
pulse_brightness = load(FUNCTIONS + "/MF_PulseBrightness")
meaning_colors = load(GENERIC + "/MF_MeaningColors")
color_scale = load(GENERIC + "/MF_ColorScale")
polar_coordinates = load(GENERIC + "/MF_PolarCoordinates")
zone_shape = load(FUNCTIONS + "/MF_ZoneShape")
pick_color = load(GENERIC + "/MF_PickColor")
stripes = load(GENERIC + "/MF_ColorPattern_Stripes")
pattern_collection = load("/Game/Art/VFX/Generic/Materials/MPC_MeaningColors")
mask_collection = load(MASK_COLLECTION)
V = unreal.MaterialExpressionVectorParameter
S = unreal.MaterialExpressionScalarParameter

# --- MF_InsideFrameOnAxis: 1 while a point is inside the frame's edges on one axis of a rectangle ---
graph = open_function(FUNCTIONS, "MF_InsideFrameOnAxis", "1 inside the edges of a rectangle's outline frame on one of "
                      "its axes: the frame is OutlineThickness wide on both sides.", CATEGORY)
from_middle = graph.input("FromMiddle", "Scalar", 0, "0 on the rectangle's middle line, 1 on either edge.")
size = graph.input("Size", "Scalar", 1, "The rectangle's size on this axis, world cm.")
thickness = graph.input("OutlineThickness", "Scalar", 2, "Width of the frame, world cm.")
both_edges = graph.node(unreal.MaterialExpressionMultiply, const_b=2.0)
graph.connect(thickness, both_edges, "A")
edge_fraction = graph.op(unreal.MaterialExpressionDivide, A=both_edges, B=size)
inner_edge = graph.op(unreal.MaterialExpressionOneMinus, edge_fraction)
# Step(Y, X) is 1 where X >= Y.
inside = graph.op(unreal.MaterialExpressionStep, Y=from_middle, X=inner_edge)
graph.output("Inside", 0, "1 inside the frame's edges on this axis.", inside)
graph.finish()
inside_frame_on_axis = graph.asset

# --- MF_BeamFrame: the beam's quad split into its outline frame and what the frame encloses ---
graph = open_function(FUNCTIONS, "MF_BeamFrame", "A beam's quad split into its outline frame and the inside it "
                      "encloses, hard-edged, with where a point sits across and along it.", CATEGORY)
uv = graph.input("UV", "Vector2", 0, "The quad's UV: U across the beam, V from its far end back to its origin.")
length = graph.input("Length", "Scalar", 1, "Beam length, world cm.")
width = graph.input("Width", "Scalar", 2, "Beam width, world cm.")
thickness = graph.input("OutlineThickness", "Scalar", 3, "Width of the frame, world cm.")
signed = graph.call(zero_one_to_signed, Value=uv)
from_middle = graph.op(unreal.MaterialExpressionAbs, (signed, "Signed"))
across = graph.mask(from_middle, "r")
inside_width = graph.call(inside_frame_on_axis, FromMiddle=across, Size=width, OutlineThickness=thickness)
inside_length = graph.call(inside_frame_on_axis, FromMiddle=graph.mask(from_middle, "g"), Size=length,
                           OutlineThickness=thickness)
inside = graph.op(unreal.MaterialExpressionMultiply, A=(inside_width, "Inside"), B=(inside_length, "Inside"))
outline = graph.op(unreal.MaterialExpressionOneMinus, inside)
along = graph.op(unreal.MaterialExpressionOneMinus, graph.mask(uv, "g"))
graph.output("Outline", 0, "1 on the frame.", outline)
graph.output("Inside", 1, "1 inside the frame.", inside)
graph.output("Across", 2, "0 on the beam's middle line, 1 on either side.", across)
graph.output("Along", 3, "0 at the beam's origin, 1 at its far end.", along)
graph.finish()
beam_frame = graph.asset

# --- MF_BeamPulse: brightness waves running to the far end ---
graph = open_function(FUNCTIONS, "MF_BeamPulse", "Brightness waves running down a beam, from its origin to its far "
                      "end.", CATEGORY)
along = graph.input("Along", "Scalar", 0, "0 at the beam's origin, 1 at its far end.")
count = graph.input("Count", "Scalar", 1, "Waves along the whole beam.")
speed = graph.input("Speed", "Scalar", 2, "Waves per second.")
time = graph.node(unreal.MaterialExpressionTime)
waves = graph.op(unreal.MaterialExpressionMultiply, A=along, B=count)
clock = graph.op(unreal.MaterialExpressionMultiply, A=time, B=speed)
travelled = graph.op(unreal.MaterialExpressionSubtract, A=waves, B=clock)
brightness = graph.call(pulse_brightness, Turns=travelled)
graph.output("Brightness", 0, "Pulse brightness, see MF_PulseBrightness.", (brightness, "Brightness"))
graph.finish()
beam_pulse = graph.asset

# --- MF_TelegraphColors: world stripes cycling through the colours, a lone colour paired with its shade ---
graph = open_function(FUNCTIONS, "MF_TelegraphColors", "The meaning colours of a telegraph: straight stripes laid over "
                      "the world at MPC_MeaningColors' stripe width and angle, each taking the next colour. A "
                      "telegraph never draws one flat colour: alone, its colour is paired with a shade of itself.",
                      "GeoTrinity|Telegraph")
count = graph.input("ColorCount", "Scalar", 0, "How many colours the telegraph carries, 1 to 4.")
colors = {f"Color{slot}": graph.input(f"Color{slot}", "Vector3", 1 + slot, f"Colour of meaning {slot + 1}.")
          for slot in range(len(COLOR_DEFAULTS))}
shade = graph.input("SingleColorShade", "Scalar", 5, "Brightness of the second shade while ColorCount is 1.")
second_shade = graph.op(unreal.MaterialExpressionMultiply, A=colors["Color0"], B=shade)
# Step(Y, X) is 1 where X >= Y.
has_second_color = graph.node(unreal.MaterialExpressionStep, const_y=1.5)
graph.connect(count, has_second_color, "X")
second_color = graph.op(unreal.MaterialExpressionLinearInterpolate, A=second_shade, B=colors["Color1"],
                        Alpha=has_second_color)
pattern_count = graph.node(unreal.MaterialExpressionMax, const_b=2.0)
graph.connect(count, pattern_count, "A")
world = graph.mask(graph.node(unreal.MaterialExpressionWorldPosition), "rg")
angle = graph.collection_parameter(pattern_collection, "TelegraphStripeAngle")
across_x = graph.node(unreal.MaterialExpressionCosine, period=360.0)
graph.connect(angle, across_x)
across_y = graph.node(unreal.MaterialExpressionSine, period=360.0)
graph.connect(angle, across_y)
direction = graph.op(unreal.MaterialExpressionAppendVector, A=across_x, B=across_y)
across = graph.op(unreal.MaterialExpressionDotProduct, A=world, B=direction)
width = graph.collection_parameter(pattern_collection, "TelegraphStripeWidth")
in_stripes = graph.op(unreal.MaterialExpressionDivide, A=across, B=width)
still = graph.node(unreal.MaterialExpressionConstant, r=0.0)
position = graph.op(unreal.MaterialExpressionAppendVector, A=in_stripes, B=still)
pattern = graph.call(stripes, Position=position, Scroll=still, ColorCount=pattern_count)
picked = graph.call(pick_color, Index=(pattern, "ColorIndex"), Color0=colors["Color0"], Color1=second_color,
                    Color2=colors["Color2"], Color3=colors["Color3"])
graph.output("Color", 0, "The colour of the point's region.", (picked, "Color"))
graph.finish()
telegraph_colors = graph.asset


# --- MPC_MaskedArea: the shadow mode and lane direction FSafeSpotMask writes beside the slots and the radius ---
scalars = list(mask_collection.get_editor_property("scalar_parameters"))
vectors = list(mask_collection.get_editor_property("vector_parameters"))
scalar_names = [str(scalar.get_editor_property("parameter_name")) for scalar in scalars]
vector_names = [str(vector.get_editor_property("parameter_name")) for vector in vectors]
if "SafeSpot_ShadowMode" not in scalar_names:
    scalar = unreal.CollectionScalarParameter()
    scalar.set_editor_property("parameter_name", "SafeSpot_ShadowMode")
    scalar.set_editor_property("default_value", 0.0)
    scalars.append(scalar)
    mask_collection.set_editor_property("scalar_parameters", scalars, toolkit["ALWAYS"])

if "SafeSpot_LaneDirection" not in vector_names:
    vector = unreal.CollectionVectorParameter()
    vector.set_editor_property("parameter_name", "SafeSpot_LaneDirection")
    vector.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))
    vectors.append(vector)
    mask_collection.set_editor_property("vector_parameters", vectors, toolkit["ALWAYS"])

toolkit["save"](mask_collection)

# --- MF_SafeSpotSlotCut: 1 where one safe spot shadows the pixel ---
graph = open_function(FUNCTIONS, "MF_SafeSpotSlotCut", "1 where one safe spot shadows a world position: its whole "
                      "strip across the spot when IsLane, its round hole when IsDisc, nothing for an unused slot.",
                      CATEGORY)
world_xy = graph.input("WorldXY", "Vector2", 0, "The pixel's world position, cm.")
spot = graph.input("Spot", "Vector4", 1, "The spot's world XY in red and green, alpha 1 when the slot is used.")
radius = graph.input("Radius", "Scalar", 2, "Radius of the hole, or half width of the strip, cm.")
is_lane = graph.input("IsLane", "Scalar", 3, "1 when the spot cuts a strip across the beam.")
is_disc = graph.input("IsDisc", "Scalar", 4, "1 when the spot cuts a round hole.")
lane_right = graph.input("LaneRight", "Vector2", 5, "Unit vector across the strip, perpendicular to its direction.")
spot_xy = graph.mask(spot, "rg")
offset = graph.op(unreal.MaterialExpressionSubtract, A=world_xy, B=spot_xy)
along_right = graph.op(unreal.MaterialExpressionDotProduct, A=offset, B=lane_right)
across = graph.op(unreal.MaterialExpressionAbs, along_right)
lane_distance = graph.op(unreal.MaterialExpressionMultiply, A=across, B=is_lane)
round_distance = graph.op(unreal.MaterialExpressionDistance, A=world_xy, B=spot_xy)
disc_distance = graph.op(unreal.MaterialExpressionMultiply, A=round_distance, B=is_disc)
distance = graph.op(unreal.MaterialExpressionAdd, A=lane_distance, B=disc_distance)
# Step(Y, X) is 1 where X >= Y: the pixel is within the radius.
within = graph.op(unreal.MaterialExpressionStep, Y=distance, X=radius)
casts_cut = graph.op(unreal.MaterialExpressionAdd, A=is_lane, B=is_disc)
shadowed = graph.op(unreal.MaterialExpressionMultiply, A=within, B=casts_cut)
slot_cut = graph.op(unreal.MaterialExpressionMultiply, A=shadowed, B=graph.mask(spot, "a"))
graph.output("SlotCut", 0, "1 where the spot shadows the position, else 0.", slot_cut)
graph.finish()
safe_spot_slot_cut = graph.asset

# --- MF_SafeSpotCut: 1 where a safe spot shadows the pixel (Lane or Disc; Behind belongs to the AOE material) ---
graph = open_function(FUNCTIONS, "MF_SafeSpotCut", "1 where one of the safe spots of MPC_MaskedArea shadows a world "
                      "position: Lane (mode 1) cuts the whole strip across the spot, Disc (mode 2) its round hole. "
                      "Behind (mode 0) and unused slots cut nothing.", CATEGORY)
world_xy = graph.input("WorldXY", "Vector2", 0, "The pixel's world position, cm.")
radius = graph.collection_parameter(mask_collection, "Pillar_Radius")
mode = graph.collection_parameter(mask_collection, "SafeSpot_ShadowMode")
lane_direction = graph.collection_parameter(mask_collection, "SafeSpot_LaneDirection")
is_lane = graph.call(is_whole_number, Value=mode, Whole=graph.node(unreal.MaterialExpressionConstant, r=1.0))
is_disc = graph.call(is_whole_number, Value=mode, Whole=graph.node(unreal.MaterialExpressionConstant, r=2.0))
lane_right_x = graph.node(unreal.MaterialExpressionMultiply, const_b=-1.0)
graph.connect(graph.mask(lane_direction, "g"), lane_right_x, "A")
lane_right = graph.op(unreal.MaterialExpressionAppendVector, A=lane_right_x, B=graph.mask(lane_direction, "r"))
slot_cuts = []
for slot in range(MASK_SLOTS):
    spot = graph.collection_parameter(mask_collection, f"PillarPosWS_{slot:02d}")
    slot_cuts.append((graph.call(safe_spot_slot_cut, WorldXY=world_xy, Spot=spot, Radius=radius,
                                 IsLane=(is_lane, "Is"), IsDisc=(is_disc, "Is"), LaneRight=lane_right), "SlotCut"))

graph.output("CutMask", 0, "1 where a safe spot shadows the position, else 0.",
             graph.fold(unreal.MaterialExpressionMax, slot_cuts))
graph.finish()
safe_spot_cut = graph.asset


def keep_outside_cuts(graph):
    """1 - CutMask at the pixel's world position when the effect opted into the mask (SafeSpotCut), the factor that
    lights what no safe spot shadows; 1 everywhere otherwise."""
    opted_in = graph.parameter(S, "SafeSpotCut", 0.0, "03 Safe spots", 0, "1 when the hazard drawing this owns the "
                               "safe spots of MPC_MaskedArea and is cut by them, 0 to ignore the mask. Bound to "
                               "User.SafeSpotCut by the system's renderer.")
    world = graph.mask(graph.node(unreal.MaterialExpressionWorldPosition), "rg")
    cut = graph.call(safe_spot_cut, WorldXY=world)
    applied_cut = graph.op(unreal.MaterialExpressionMultiply, A=(cut, "CutMask"), B=opted_in)
    return graph.op(unreal.MaterialExpressionOneMinus, applied_cut)


def meaning_color_parameters(graph):
    """Color, Color2-4 and ColorCount, the names the systems bind their User parameters to; returns the colours'
    RGB by MF_MeaningColors pin, and the count."""
    colors = {}
    for slot, default in enumerate(COLOR_DEFAULTS):
        name = "Color" if slot == 0 else f"Color{slot + 1}"
        description = (f"Colour {slot + 1}, shown once ColorCount reaches {slot + 1}; also the outline's colour."
                       if slot == 0 else f"Colour {slot + 1}, shown once ColorCount reaches {slot + 1}.")
        parameter = graph.parameter(V, name, unreal.LinearColor(*default, 1.0), "04 Colour", slot, description
                                    + f" Bound to User.{name} by the system's renderer.")
        colors[f"Color{slot}"] = graph.mask(parameter, "rgb")

    count = graph.parameter(S, "ColorCount", 1.0, "04 Colour", len(COLOR_DEFAULTS), "How many of the colours the "
                            "fill shows, 1 to 4. Bound to User.ColorCount by the system's renderer.")
    return colors, count


def telegraph_fill_color(graph, colors, color_count):
    shade = graph.parameter(S, "SingleColorShade", 0.35, "04 Colour", len(COLOR_DEFAULTS) + 1, "Brightness of the "
                            "pattern's second shade while ColorCount is 1.")
    return graph.call(telegraph_colors, ColorCount=color_count, SingleColorShade=shade, **colors)


# --- M_PulseBeam: outline frame, pulse running to the far end, fill in the meaning colours ---
graph = toolkit["open_material"](FOLDER, "M_PulseBeam")
uv = graph.node(unreal.MaterialExpressionTextureCoordinate)
# Written per particle by the beam emitter's dynamic material parameters, index 0. Its outputs connect by channel:
# they take their ParamNames only in the material editor.
beam = graph.node(unreal.MaterialExpressionDynamicParameter, param_names=["Length", "Width", "DurationSpent", "Param4"],
                  default_value=unreal.LinearColor(2.0, 2.0, 0.3, 1.0))

# Shape
thickness = graph.parameter(S, "OutlineThickness", 2.0, "01 Shape", 0, "Width of the outline frame, world cm.")
shape = graph.call(beam_frame, UV=uv, Length=(beam, "R"), Width=(beam, "G"),
                   OutlineThickness=thickness)

# Pulse and life
pulse_count = graph.parameter(S, "PulseCount", 3.0, "02 Pulse", 0, "Brightness waves along the whole beam.")
pulse_speed = graph.parameter(S, "PulseSpeed", 1.0, "02 Pulse", 1, "Brightness waves per second, running to the far "
                              "end.")
pulse = graph.call(beam_pulse, Along=(shape, "Along"), Count=pulse_count, Speed=pulse_speed)
# Step(Y, X) is 1 where X >= Y: lit from the origin out to DurationSpent.
wipe = graph.op(unreal.MaterialExpressionStep, Y=(shape, "Along"), X=(beam, "B"))
life = graph.node(unreal.MaterialExpressionAdd, const_b=SPENT_BRIGHTNESS)
graph.connect(wipe, life, "A")
life_brightness = graph.op(unreal.MaterialExpressionSaturate, life)

# Look: the particle colour carries the emitter's fade, so every colour is scaled by how far it has faded.
colors, color_count = meaning_color_parameters(graph)
fill_color = graph.call(meaning_colors, ColorCount=color_count, **colors)
particle = graph.node(unreal.MaterialExpressionParticleColor)
fade = graph.call(color_scale, Scaled=graph.mask(particle, "rgb"), Original=colors["Color0"])

# Outputs
lit_inside = graph.op(unreal.MaterialExpressionMultiply, A=(shape, "Inside"), B=(pulse, "Brightness"))
fill = graph.op(unreal.MaterialExpressionMultiply, A=lit_inside, B=life_brightness)
fill_light = graph.op(unreal.MaterialExpressionMultiply, A=(fill_color, "Color"), B=fill)
outline_light = graph.op(unreal.MaterialExpressionMultiply, A=colors["Color0"], B=(shape, "Outline"))
light = graph.op(unreal.MaterialExpressionAdd, A=outline_light, B=fill_light)
faded = graph.op(unreal.MaterialExpressionMultiply, A=light, B=(fade, "Scale"))
emissive = graph.op(unreal.MaterialExpressionMultiply, A=faded, B=keep_outside_cuts(graph))
graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
graph.finish()

# --- M_ZoneIndicatorRay: outline frame, fill growing from the middle line, in the meaning colours ---
graph = toolkit["open_material"](FOLDER, "M_ZoneIndicatorRay")
uv = graph.node(unreal.MaterialExpressionTextureCoordinate)
# Written per particle by the indicator emitter: its colour and opacity at index 0, the telegraph at index 1. Their
# outputs connect by channel: they take their ParamNames only in the material editor.
tint = graph.node(unreal.MaterialExpressionDynamicParameter, param_names=["Red", "Green", "Blue", "Alpha"],
                  default_value=unreal.LinearColor(0.0, 0.434, 1.0, 1.0))
ray = graph.node(unreal.MaterialExpressionDynamicParameter, parameter_index=1,
                 param_names=["FillAlpha", "Length", "Width", "Param4"],
                 default_value=unreal.LinearColor(0.48, 100.0, 100.0, 1.0))

# Shape
thickness = graph.parameter(S, "LineThickness", 5.0, "01 Shape", 0, "Width of the outline frame, world cm.")
shape = graph.call(beam_frame, UV=uv, Length=(ray, "G"), Width=(ray, "B"),
                   OutlineThickness=thickness)
# Step(Y, X) is 1 where X >= Y: FillAlpha 0 to 1 grows the fill from the middle line out to both sides.
fill = graph.op(unreal.MaterialExpressionStep, Y=(shape, "Across"), X=(ray, "R"))

# Look
colors, color_count = meaning_color_parameters(graph)
fill_color = telegraph_fill_color(graph, colors, color_count)

# Outputs
fill_light = graph.op(unreal.MaterialExpressionMultiply, A=(fill_color, "Color"), B=fill)
light = graph.op(unreal.MaterialExpressionLinearInterpolate, A=fill_light, B=colors["Color0"],
                 Alpha=(shape, "Outline"))
keep = keep_outside_cuts(graph)
emissive = graph.op(unreal.MaterialExpressionMultiply, A=light, B=keep)
coverage = graph.op(unreal.MaterialExpressionMax, A=(shape, "Outline"), B=fill)
visible = graph.op(unreal.MaterialExpressionMultiply, A=coverage, B=(tint, "A"))
opacity = graph.op(unreal.MaterialExpressionMultiply, A=visible, B=keep)
graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
graph.to_property(opacity, unreal.MaterialProperty.MP_OPACITY)
graph.finish()

# --- M_ZoneIndicator: outline ring, fill disc growing from the centre, the zone's star in the meaning colours ---
graph = toolkit["open_material"](FOLDER, "M_ZoneIndicator")
uv = graph.node(unreal.MaterialExpressionTextureCoordinate)
# Written per particle by the indicator emitter: the fill's radius at index 0, the ring's thickness at index 1, both
# fractions of the circle's radius, on alpha. Their outputs connect by channel: they take their ParamNames only in the
# material editor.
fill_radius = graph.node(unreal.MaterialExpressionDynamicParameter, param_names=["Param1", "Param2", "Param3", "FillRadius"],
                         default_value=unreal.LinearColor(1.0, 1.0, 1.0, 0.368))
ring = graph.node(unreal.MaterialExpressionDynamicParameter, parameter_index=1,
                  param_names=["Param1", "Param2", "Param3", "LineThickness"],
                  default_value=unreal.LinearColor(1.0, 1.0, 1.0, 0.028))

# Space: the quad centred to [-1, 1], as distance from the centre and angle around it.
quad = graph.call(zero_one_to_signed, Value=uv)
polar = graph.call(polar_coordinates, Position=(quad, "Signed"))
radius = (polar, "Radius")

# Shape
shape = graph.call(zone_shape, Radius=radius, OutlineThickness=(ring, "A"))
# Step(Y, X) is 1 where X >= Y: the fill reaches out to its radius.
fill = graph.op(unreal.MaterialExpressionStep, Y=radius, X=(fill_radius, "A"))
fill_opacity = graph.parameter(S, "FillOpacity", 0.5, "01 Shape", 0, "Opacity of the fill, against the ring's 1.")

# Look
colors, color_count = meaning_color_parameters(graph)
fill_color = telegraph_fill_color(graph, colors, color_count)
brightness = graph.parameter(S, "Brightness", 1.0, "04 Colour", len(COLOR_DEFAULTS) + 2, "Emissive multiplier of "
                             "every colour.")

# Outputs
fill_light = graph.op(unreal.MaterialExpressionMultiply, A=(fill_color, "Color"), B=fill)
light = graph.op(unreal.MaterialExpressionLinearInterpolate, A=fill_light, B=colors["Color0"],
                 Alpha=(shape, "Outline"))
emissive = graph.op(unreal.MaterialExpressionMultiply, A=light, B=brightness)
faint_fill = graph.op(unreal.MaterialExpressionMultiply, A=fill, B=fill_opacity)
opacity = graph.op(unreal.MaterialExpressionMax, A=(shape, "Outline"), B=faint_fill)
graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
graph.to_property(opacity, unreal.MaterialProperty.MP_OPACITY)
graph.finish()

unreal.log(f"BEAMMATERIALS::built {FUNCTIONS}/MF_SafeSpotCut, MF_BeamFrame, {FOLDER}/M_PulseBeam, M_ZoneIndicatorRay and M_ZoneIndicator")
