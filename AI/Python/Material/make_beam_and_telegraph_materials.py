"""The beam and its windup telegraph, and the round zone telegraph, rebuilt from shared functions: M_PulseBeam,
M_ZoneIndicatorRay and M_ZoneIndicator. Each draws its fill in up to four colours through MF_MeaningColors, whose
pattern lies over the world, the same on every shape. Rebuilt in place, so every instance and emitter keeps them.

Run make_color_pattern_functions.py and make_pulse_circle_material.py first. The Color, Color2-4 and ColorCount
parameters are bound to the systems' User parameters by AI/Python/Niagara/meaning_color_bindings.py: renaming one here
means renaming it there."""
import unreal

FOLDER = "/Game/Art/VFX/AOE"
FUNCTIONS = FOLDER + "/Functions"
GENERIC = "/Game/Art/VFX/Generic/Materials/Functions"
CATEGORY = "GeoTrinity|Beam"
PULSE_MIDDLE = 0.65
PULSE_SWING = 0.2
SPENT_BRIGHTNESS = 0.2  # of the fill, on the part of the beam's life not yet wiped
COLOR_DEFAULTS = [(0.15, 0.6, 1.0), (1.0, 0.15, 0.1), (0.2, 1.0, 0.3), (1.0, 0.85, 0.1)]

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
load = toolkit["load"]
open_function = toolkit["open_function"]
meaning_colors = load(GENERIC + "/MF_MeaningColors")
color_scale = load(GENERIC + "/MF_ColorScale")
polar_coordinates = load(GENERIC + "/MF_PolarCoordinates")
zone_shape = load(FUNCTIONS + "/MF_ZoneShape")
pick_color = load(GENERIC + "/MF_PickColor")
stripes = load(GENERIC + "/MF_ColorPattern_Stripes")
pattern_collection = load("/Game/Art/VFX/Generic/Materials/MPC_MeaningColors")
V = unreal.MaterialExpressionVectorParameter
S = unreal.MaterialExpressionScalarParameter

# --- MF_BeamShape: the beam's quad split into its outline frame and what the frame encloses ---
graph = open_function(FUNCTIONS, "MF_BeamShape", "A beam's quad split into its outline frame and the inside it "
                      "encloses, hard-edged, with where a point sits across and along it.", CATEGORY)
uv = graph.input("UV", "Vector2", 0, "The quad's UV: U across the beam, V from its far end back to its origin.", -1100,
                 -40)
length = graph.input("Length", "Scalar", 1, "Beam length, world cm.", -1100, 200)
width = graph.input("Width", "Scalar", 2, "Beam width, world cm.", -1100, 320)
thickness = graph.input("OutlineThickness", "Scalar", 3, "Width of the frame, world cm.", -1100, 440)
u = graph.mask(uv, "r", -900, -80)
v = graph.mask(uv, "g", -900, 40)


def distance_from_middle(coordinate, y):
    """0 on the middle line, 1 on either edge."""
    centred = graph.node(unreal.MaterialExpressionSubtract, -760, y, const_b=0.5)
    graph.connect(coordinate, centred, "A")
    doubled = graph.node(unreal.MaterialExpressionMultiply, -640, y, const_b=2.0)
    graph.connect(centred, doubled, "A")
    return graph.op(unreal.MaterialExpressionAbs, -520, y, doubled)


def inside_along_axis(distance, size, y):
    """Step(Y, X) is 1 where X >= Y: inside the frame's edges on this axis, 1 - 2 * thickness / size out."""
    both_edges = graph.node(unreal.MaterialExpressionMultiply, -760, y + 60, const_b=2.0)
    graph.connect(thickness, both_edges, "A")
    edge_fraction = graph.op(unreal.MaterialExpressionDivide, -640, y + 60, A=both_edges, B=size)
    inner_edge = graph.op(unreal.MaterialExpressionOneMinus, -520, y + 60, edge_fraction)
    return graph.op(unreal.MaterialExpressionStep, -380, y, Y=distance, X=inner_edge)


across = distance_from_middle(u, -80)
inside = graph.op(unreal.MaterialExpressionMultiply, -220, 0, A=inside_along_axis(across, width, -80),
                  B=inside_along_axis(distance_from_middle(v, 160), length, 160))
outline = graph.op(unreal.MaterialExpressionOneMinus, -80, -80, inside)
along = graph.op(unreal.MaterialExpressionOneMinus, -80, 240, v)
graph.output("Outline", 0, "1 on the frame.", outline, 80, -120)
graph.output("Inside", 1, "1 inside the frame.", inside, 80, 0)
graph.output("Across", 2, "0 on the beam's middle line, 1 on either side.", across, 80, 120)
graph.output("Along", 3, "0 at the beam's origin, 1 at its far end.", along, 80, 240)
graph.finish()
beam_shape = graph.asset

# --- MF_BeamPulse: sin(Along * Count - Time * Speed) brightness waves running to the far end ---
graph = open_function(FUNCTIONS, "MF_BeamPulse", "Brightness waves running down a beam, from its origin to its far "
                      "end.", CATEGORY)
along = graph.input("Along", "Scalar", 0, "0 at the beam's origin, 1 at its far end.", -800, -80)
count = graph.input("Count", "Scalar", 1, "Waves along the whole beam.", -800, 40)
speed = graph.input("Speed", "Scalar", 2, "Waves per second.", -800, 160)
time = graph.node(unreal.MaterialExpressionTime, -800, 280)
waves = graph.op(unreal.MaterialExpressionMultiply, -620, -40, A=along, B=count)
clock = graph.op(unreal.MaterialExpressionMultiply, -620, 200, A=time, B=speed)
travelled = graph.op(unreal.MaterialExpressionSubtract, -480, 40, A=waves, B=clock)
# Sine's default period of 1 reads its input in turns.
wave = graph.op(unreal.MaterialExpressionSine, -360, 40, travelled)
swing = graph.node(unreal.MaterialExpressionMultiply, -240, 40, const_b=PULSE_SWING)
graph.connect(wave, swing, "A")
brightness = graph.node(unreal.MaterialExpressionAdd, -120, 40, const_b=PULSE_MIDDLE)
graph.connect(swing, brightness, "A")
graph.output("Brightness", 0, f"{PULSE_MIDDLE - PULSE_SWING:.2f} to {PULSE_MIDDLE + PULSE_SWING:.2f}.", brightness,
             20, 40)
graph.finish()
beam_pulse = graph.asset

# --- MF_TelegraphColors: world stripes cycling through the colours, a lone colour paired with its shade ---
graph = open_function(FUNCTIONS, "MF_TelegraphColors", "The meaning colours of a telegraph: straight stripes laid over "
                      "the world at MPC_MeaningColors' stripe width and angle, each taking the next colour. A "
                      "telegraph never draws one flat colour: alone, its colour is paired with a shade of itself.",
                      "GeoTrinity|Telegraph")
count = graph.input("ColorCount", "Scalar", 0, "How many colours the telegraph carries, 1 to 4.", -1000, -100)
colors = {f"Color{slot}": graph.input(f"Color{slot}", "Vector3", 1 + slot, f"Colour of meaning {slot + 1}.", -1000,
                                      slot * 120)
          for slot in range(len(COLOR_DEFAULTS))}
shade = graph.input("SingleColorShade", "Scalar", 5, "Brightness of the second shade while ColorCount is 1.", -1000,
                    500)
second_shade = graph.op(unreal.MaterialExpressionMultiply, -760, 400, A=colors["Color0"], B=shade)
# Step(Y, X) is 1 where X >= Y.
has_second_color = graph.node(unreal.MaterialExpressionStep, -760, -60, const_y=1.5)
graph.connect(count, has_second_color, "X")
second_color = graph.op(unreal.MaterialExpressionLinearInterpolate, -580, 200, A=second_shade, B=colors["Color1"],
                        Alpha=has_second_color)
pattern_count = graph.node(unreal.MaterialExpressionMax, -580, -100, const_b=2.0)
graph.connect(count, pattern_count, "A")
world = graph.mask(graph.node(unreal.MaterialExpressionWorldPosition, -1400, -500), "rg", -1240, -500)
angle = graph.collection_parameter(pattern_collection, "TelegraphStripeAngle", -1400, -380)
across_x = graph.node(unreal.MaterialExpressionCosine, -1240, -400, period=360.0)
graph.connect(angle, across_x)
across_y = graph.node(unreal.MaterialExpressionSine, -1240, -320, period=360.0)
graph.connect(angle, across_y)
direction = graph.op(unreal.MaterialExpressionAppendVector, -1100, -360, A=across_x, B=across_y)
across = graph.op(unreal.MaterialExpressionDotProduct, -960, -440, A=world, B=direction)
width = graph.collection_parameter(pattern_collection, "TelegraphStripeWidth", -1400, -240)
in_stripes = graph.op(unreal.MaterialExpressionDivide, -820, -400, A=across, B=width)
still = graph.node(unreal.MaterialExpressionConstant, -820, -280, r=0.0)
position = graph.op(unreal.MaterialExpressionAppendVector, -680, -360, A=in_stripes, B=still)
pattern = graph.call(stripes, -520, -200, Position=position, Scroll=still, ColorCount=pattern_count)
picked = graph.call(pick_color, -380, 0, Index=(pattern, "ColorIndex"), Color0=colors["Color0"], Color1=second_color,
                    Color2=colors["Color2"], Color3=colors["Color3"])
graph.output("Color", 0, "The colour of the point's region.", (picked, "Color"), -120, 0)
graph.finish()
telegraph_colors = graph.asset


def meaning_color_parameters(graph, x, y):
    """Color, Color2-4 and ColorCount, the names the systems bind their User parameters to; returns the colours'
    RGB by MF_MeaningColors pin, and the count."""
    colors = {}
    for slot, default in enumerate(COLOR_DEFAULTS):
        name = "Color" if slot == 0 else f"Color{slot + 1}"
        description = (f"Colour {slot + 1}, shown once ColorCount reaches {slot + 1}; also the outline's colour."
                       if slot == 0 else f"Colour {slot + 1}, shown once ColorCount reaches {slot + 1}.")
        parameter = graph.parameter(V, name, unreal.LinearColor(*default, 1.0), "04 Colour", slot, description
                                    + f" Bound to User.{name} by the system's renderer.", x, y + slot * 140)
        colors[f"Color{slot}"] = graph.mask(parameter, "rgb", x + 220, y + slot * 140)

    count = graph.parameter(S, "ColorCount", 1.0, "04 Colour", len(COLOR_DEFAULTS), "How many of the colours the "
                            "fill shows, 1 to 4. Bound to User.ColorCount by the system's renderer.", x,
                            y + len(COLOR_DEFAULTS) * 140)
    return colors, count


def telegraph_fill_color(graph, colors, color_count, x, y):
    shade = graph.parameter(S, "SingleColorShade", 0.35, "04 Colour", len(COLOR_DEFAULTS) + 1, "Brightness of the "
                            "pattern's second shade while ColorCount is 1.", x, y)
    return graph.call(telegraph_colors, x + 800, y - 900, ColorCount=color_count, SingleColorShade=shade, **colors)


# --- M_PulseBeam: outline frame, pulse running to the far end, fill in the meaning colours ---
graph = toolkit["open_material"](FOLDER, "M_PulseBeam")
uv = graph.node(unreal.MaterialExpressionTextureCoordinate, -2400, 0)
# Written per particle by the beam emitter's dynamic material parameters, index 0. Its outputs connect by channel:
# they take their ParamNames only in the material editor.
beam = graph.node(unreal.MaterialExpressionDynamicParameter, -2400, 200,
                  param_names=["Length", "Width", "DurationSpent", "Param4"],
                  default_value=unreal.LinearColor(2.0, 2.0, 0.3, 1.0))

# Shape
thickness = graph.parameter(S, "OutlineThickness", 2.0, "01 Shape", 0, "Width of the outline frame, world cm.",
                            -2100, -400)
shape = graph.call(beam_shape, -1800, -400, UV=uv, Length=(beam, "R"), Width=(beam, "G"),
                   OutlineThickness=thickness)

# Pulse and life
pulse_count = graph.parameter(S, "PulseCount", 3.0, "02 Pulse", 0, "Brightness waves along the whole beam.", -2100,
                              -160)
pulse_speed = graph.parameter(S, "PulseSpeed", 1.0, "02 Pulse", 1, "Brightness waves per second, running to the far "
                              "end.", -2100, -60)
pulse = graph.call(beam_pulse, -1500, -160, Along=(shape, "Along"), Count=pulse_count, Speed=pulse_speed)
# Step(Y, X) is 1 where X >= Y: lit from the origin out to DurationSpent.
wipe = graph.op(unreal.MaterialExpressionStep, -1360, 0, Y=(shape, "Along"), X=(beam, "B"))
life = graph.node(unreal.MaterialExpressionAdd, -1220, 0, const_b=SPENT_BRIGHTNESS)
graph.connect(wipe, life, "A")
life_brightness = graph.op(unreal.MaterialExpressionSaturate, -1100, 0, life)

# Look: the particle colour carries the emitter's fade, so every colour is scaled by how far it has faded.
colors, color_count = meaning_color_parameters(graph, -2100, 640)
fill_color = graph.call(meaning_colors, -1500, 400, ColorCount=color_count, **colors)
particle = graph.node(unreal.MaterialExpressionParticleColor, -1500, 700)
fade = graph.call(color_scale, -1200, 700, Scaled=graph.mask(particle, "rgb", -1360, 700), Original=colors["Color0"])

# Outputs
lit_inside = graph.op(unreal.MaterialExpressionMultiply, -1100, -200, A=(shape, "Inside"), B=(pulse, "Brightness"))
fill = graph.op(unreal.MaterialExpressionMultiply, -940, -100, A=lit_inside, B=life_brightness)
fill_light = graph.op(unreal.MaterialExpressionMultiply, -800, 200, A=(fill_color, "Color"), B=fill)
outline_light = graph.op(unreal.MaterialExpressionMultiply, -800, -400, A=colors["Color0"], B=(shape, "Outline"))
light = graph.op(unreal.MaterialExpressionAdd, -640, 0, A=outline_light, B=fill_light)
emissive = graph.op(unreal.MaterialExpressionMultiply, -480, 0, A=light, B=(fade, "Scale"))
graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
graph.finish()

# --- M_ZoneIndicatorRay: outline frame, fill growing from the middle line, in the meaning colours ---
graph = toolkit["open_material"](FOLDER, "M_ZoneIndicatorRay")
uv = graph.node(unreal.MaterialExpressionTextureCoordinate, -2400, 0)
# Written per particle by the indicator emitter: its colour and opacity at index 0, the telegraph at index 1. Their
# outputs connect by channel: they take their ParamNames only in the material editor.
tint = graph.node(unreal.MaterialExpressionDynamicParameter, -2400, 560, param_names=["Red", "Green", "Blue", "Alpha"],
                  default_value=unreal.LinearColor(0.0, 0.434, 1.0, 1.0))
ray = graph.node(unreal.MaterialExpressionDynamicParameter, -2400, 200, parameter_index=1,
                 param_names=["FillAlpha", "Length", "Width", "Param4"],
                 default_value=unreal.LinearColor(0.48, 100.0, 100.0, 1.0))

# Shape
thickness = graph.parameter(S, "LineThickness", 5.0, "01 Shape", 0, "Width of the outline frame, world cm.", -2100,
                            -400)
shape = graph.call(beam_shape, -1800, -400, UV=uv, Length=(ray, "G"), Width=(ray, "B"),
                   OutlineThickness=thickness)
# Step(Y, X) is 1 where X >= Y: FillAlpha 0 to 1 grows the fill from the middle line out to both sides.
fill = graph.op(unreal.MaterialExpressionStep, -1500, -200, Y=(shape, "Across"), X=(ray, "R"))

# Look
colors, color_count = meaning_color_parameters(graph, -2100, 640)
fill_color = telegraph_fill_color(graph, colors, color_count, -2100, 1340)

# Outputs
fill_light = graph.op(unreal.MaterialExpressionMultiply, -1000, 200, A=(fill_color, "Color"), B=fill)
emissive = graph.op(unreal.MaterialExpressionLinearInterpolate, -800, 0, A=fill_light, B=colors["Color0"],
                    Alpha=(shape, "Outline"))
coverage = graph.op(unreal.MaterialExpressionMax, -1000, -300, A=(shape, "Outline"), B=fill)
opacity = graph.op(unreal.MaterialExpressionMultiply, -800, -300, A=coverage, B=(tint, "A"))
graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
graph.to_property(opacity, unreal.MaterialProperty.MP_OPACITY)
graph.finish()

# --- M_ZoneIndicator: outline ring, fill disc growing from the centre, the zone's star in the meaning colours ---
graph = toolkit["open_material"](FOLDER, "M_ZoneIndicator")
uv = graph.node(unreal.MaterialExpressionTextureCoordinate, -2400, 0)
# Written per particle by the indicator emitter: the fill's radius at index 0, the ring's thickness at index 1, both
# fractions of the circle's radius, on alpha. Their outputs connect by channel: they take their ParamNames only in the
# material editor.
fill_radius = graph.node(unreal.MaterialExpressionDynamicParameter, -2400, 400,
                         param_names=["Param1", "Param2", "Param3", "FillRadius"],
                         default_value=unreal.LinearColor(1.0, 1.0, 1.0, 0.368))
ring = graph.node(unreal.MaterialExpressionDynamicParameter, -2400, 600, parameter_index=1,
                  param_names=["Param1", "Param2", "Param3", "LineThickness"],
                  default_value=unreal.LinearColor(1.0, 1.0, 1.0, 0.028))

# Space: the quad centred to [-1, 1], as distance from the centre and angle around it.
centred = graph.node(unreal.MaterialExpressionSubtract, -2240, 0, const_b=0.5)
graph.connect(uv, centred, "A")
quad = graph.node(unreal.MaterialExpressionMultiply, -2100, 0, const_b=2.0)
graph.connect(centred, quad, "A")
polar = graph.call(polar_coordinates, -1940, 0, Position=quad)
radius = (polar, "Radius")

# Shape
shape = graph.call(zone_shape, -1600, -400, Radius=radius, OutlineThickness=(ring, "A"))
# Step(Y, X) is 1 where X >= Y: the fill reaches out to its radius.
fill = graph.op(unreal.MaterialExpressionStep, -1600, -200, Y=radius, X=(fill_radius, "A"))
fill_opacity = graph.parameter(S, "FillOpacity", 0.5, "01 Shape", 0, "Opacity of the fill, against the ring's 1.",
                               -1600, -80)

# Look
colors, color_count = meaning_color_parameters(graph, -2100, 640)
fill_color = telegraph_fill_color(graph, colors, color_count, -2100, 1340)
brightness = graph.parameter(S, "Brightness", 1.0, "04 Colour", len(COLOR_DEFAULTS) + 2, "Emissive multiplier of "
                             "every colour.", -1000, 600)

# Outputs
fill_light = graph.op(unreal.MaterialExpressionMultiply, -1000, 200, A=(fill_color, "Color"), B=fill)
light = graph.op(unreal.MaterialExpressionLinearInterpolate, -800, 0, A=fill_light, B=colors["Color0"],
                 Alpha=(shape, "Outline"))
emissive = graph.op(unreal.MaterialExpressionMultiply, -640, 0, A=light, B=brightness)
faint_fill = graph.op(unreal.MaterialExpressionMultiply, -1000, -200, A=fill, B=fill_opacity)
opacity = graph.op(unreal.MaterialExpressionMax, -800, -300, A=(shape, "Outline"), B=faint_fill)
graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
graph.to_property(opacity, unreal.MaterialProperty.MP_OPACITY)
graph.finish()

unreal.log(f"BEAMMATERIALS::built {FOLDER}/M_PulseBeam, M_ZoneIndicatorRay and M_ZoneIndicator")
