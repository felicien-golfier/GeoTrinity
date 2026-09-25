"""M_PulseCircle, every zone's disc: an outline ring and a pulsing fill drawn in up to four colours through
MF_MeaningColors, dimmed where the zone's life is spent. Rebuilt in place, so every instance and mesh keeps it.

Run make_color_pattern_functions.py first. The parameter names C++ writes are declared in GeoMaterialParams
(Source/GeoTrinity/Public/Tool/GeoNiagaraParams.h): renaming one here means renaming it there."""
import unreal

FOLDER = "/Game/Art/VFX/AOE"
FUNCTIONS = FOLDER + "/Functions"
GENERIC = "/Game/Art/VFX/Generic/Materials/Functions"
CATEGORY = "GeoTrinity|Zone"
PULSE_PHASE_KEY = (0.13731, 0.27113, 0.05777)  # any irrational-ish direction: neighbouring zones pulse out of step
PULSE_MIDDLE = 0.65
PULSE_SWING = 0.2
SPENT_BRIGHTNESS = 0.3  # of the fill, on the part of the zone's life already used up

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
load = toolkit["load"]
open_function = toolkit["open_function"]
polar_coordinates = load(GENERIC + "/MF_PolarCoordinates")
duration_wipe = load(GENERIC + "/MF_DurationWipe")
meaning_colors = load(GENERIC + "/MF_MeaningColors")

# --- MF_ZoneShape: the disc, its outline ring and what the ring encloses ---
graph = open_function(FUNCTIONS, "MF_ZoneShape", "A zone's disc split into its outline ring and the inside it "
                      "encloses, hard-edged.", CATEGORY)
radius = graph.input("Radius", "Scalar", 0, "Distance from the zone's centre, 1 on its edge.", -700, -40)
thickness = graph.input("OutlineThickness", "Scalar", 1, "Width of the ring, as a fraction of the radius.", -700, 120)
# Step(Y, X) is 1 where X >= Y.
disc = graph.node(unreal.MaterialExpressionStep, -440, -120, const_x=1.0)
graph.connect(radius, disc, "Y")
inner_edge = graph.op(unreal.MaterialExpressionOneMinus, -500, 120, thickness)
inside = graph.op(unreal.MaterialExpressionStep, -300, 40, Y=radius, X=inner_edge)
outline = graph.op(unreal.MaterialExpressionSubtract, -140, -80, A=disc, B=inside)
graph.output("Disc", 0, "1 on the whole disc.", disc, 40, -200)
graph.output("Outline", 1, "1 on the ring.", outline, 40, -80)
graph.output("Inside", 2, "1 inside the ring.", inside, 40, 40)
graph.finish()
zone_shape = graph.asset

# --- MF_ZonePulse: sin(Radius + Time * Speed + phase) brightness waves running inward ---
graph = open_function(FUNCTIONS, "MF_ZonePulse", "Brightness waves running from a zone's edge to its centre, each "
                      "zone out of step with its neighbours.", CATEGORY)
radius = graph.input("Radius", "Scalar", 0, "Distance from the zone's centre, 1 on its edge.", -1000, -80)
speed = graph.input("Speed", "Scalar", 1, "Waves per second.", -1000, 80)
time = graph.node(unreal.MaterialExpressionTime, -1000, 200)
clock = graph.op(unreal.MaterialExpressionMultiply, -820, 120, A=time, B=speed)
object_position = graph.node(unreal.MaterialExpressionObjectPositionWS, -1000, 320)
phase_key = graph.node(unreal.MaterialExpressionConstant3Vector, -1000, 420,
                       constant=unreal.LinearColor(*PULSE_PHASE_KEY, 0.0))
phase_seed = graph.op(unreal.MaterialExpressionDotProduct, -820, 360, A=object_position, B=phase_key)
phase = graph.op(unreal.MaterialExpressionFrac, -680, 360, phase_seed)
start = graph.op(unreal.MaterialExpressionAdd, -540, 200, A=clock, B=phase)
travelled = graph.op(unreal.MaterialExpressionAdd, -400, 0, A=radius, B=start)
# Sine's default period of 1 reads its input in turns.
wave = graph.op(unreal.MaterialExpressionSine, -280, 0, travelled)
swing = graph.node(unreal.MaterialExpressionMultiply, -160, 0, const_b=PULSE_SWING)
graph.connect(wave, swing, "A")
brightness = graph.node(unreal.MaterialExpressionAdd, -40, 0, const_b=PULSE_MIDDLE)
graph.connect(swing, brightness, "A")
graph.output("Brightness", 0, f"{PULSE_MIDDLE - PULSE_SWING:.2f} to {PULSE_MIDDLE + PULSE_SWING:.2f}.", brightness,
             100, 0)
graph.finish()
zone_pulse = graph.asset

# --- M_PulseCircle ---
graph = toolkit["open_material"](FOLDER, "M_PulseCircle")
V = unreal.MaterialExpressionVectorParameter
S = unreal.MaterialExpressionScalarParameter

# Space: the quad centred to [-1, 1], as distance from the centre and angle around it.
uv = graph.node(unreal.MaterialExpressionTextureCoordinate, -2400, 0)
centred = graph.node(unreal.MaterialExpressionSubtract, -2240, 0, const_b=0.5)
graph.connect(uv, centred, "A")
quad = graph.node(unreal.MaterialExpressionMultiply, -2100, 0, const_b=2.0)
graph.connect(centred, quad, "A")
polar = graph.call(polar_coordinates, -1940, 0, Position=quad)
radius = (polar, "Radius")

# Shape
thickness = graph.parameter(S, "OutlineThickness", 0.08, "01 Shape", 0, "Width of the outline ring, as a fraction "
                            "of the zone's radius.", -1600, -400)
shape = graph.call(zone_shape, -1300, -400, Radius=radius, OutlineThickness=thickness)

# Pulse and life
pulse_speed = graph.parameter(S, "PulseSpeed", 1.0734119415283203, "02 Pulse", 0, "Brightness waves per second, "
                              "running inward across the fill.", -1600, -200)
pulse = graph.call(zone_pulse, -1300, -200, Radius=radius, Speed=pulse_speed)
spent = graph.parameter(S, "DurationSpent", 0.5, "05 Duration", 0, "Fraction of the zone's life used up, 0 to 1. "
                        "Read from custom primitive data slot 0, which AGeoDeployableBase writes every tick.",
                        -1600, 0)
spent.set_editor_property("use_custom_primitive_data", True)
spent.set_editor_property("primitive_data_index", 0)
wipe = graph.call(duration_wipe, -1300, 0, UV=uv, Spent=spent)
life = graph.node(unreal.MaterialExpressionAdd, -1060, 0, const_b=SPENT_BRIGHTNESS)
graph.connect((wipe, "Wipe"), life, "A")
life_brightness = graph.op(unreal.MaterialExpressionSaturate, -940, 0, life)

# Pattern: its size and speed are MPC_MeaningColors', the same on every effect.
color_count = graph.parameter(S, "ColorCount", 1.0, "03 Pattern", 0, "How many of the Inside colours the fill "
                              "shows, 1 to 4. AGeoEffectZone writes it: Color plus its SecondaryColors.", -1600, 240)

# Look
outline_color = graph.parameter(V, "OutlineColor", unreal.LinearColor(1.0, 1.0, 1.0, 1.0), "04 Colour", 0,
                                "Colour of the outline ring. AGeoEffectZone writes the zone's Color here.", -1060, -560)
inside_colors = {}
defaults = [(0.15, 0.6, 1.0), (1.0, 0.15, 0.1), (0.2, 1.0, 0.3), (1.0, 0.85, 0.1)]
for slot, default in enumerate(defaults):
    name = "InsideColor" if slot == 0 else f"InsideColor{slot + 1}"
    description = ("Fill colour. AGeoEffectZone writes the zone's Color here." if slot == 0 else
                   f"Fill colour {slot + 1}, shown once ColorCount reaches {slot + 1}. AGeoEffectZone writes the "
                   f"zone's SecondaryColors from here on.")
    parameter = graph.parameter(V, name, unreal.LinearColor(*default, 1.0), "04 Colour", slot + 1, description,
                                -1060, 560 + slot * 140)
    inside_colors[f"Color{slot}"] = graph.mask(parameter, "rgb", -840, 580 + slot * 140)

fill_color = graph.call(meaning_colors, -640, 400, ColorCount=color_count, **inside_colors)

# Outputs
lit_inside = graph.op(unreal.MaterialExpressionMultiply, -640, -200, A=(shape, "Inside"), B=(pulse, "Brightness"))
fill = graph.op(unreal.MaterialExpressionMultiply, -480, -100, A=lit_inside, B=life_brightness)
fill_light = graph.op(unreal.MaterialExpressionMultiply, -320, 200, A=(fill_color, "Color"), B=fill)
outline_rgb = graph.mask(outline_color, "rgb", -840, -560)
outline_light = graph.op(unreal.MaterialExpressionMultiply, -320, -400, A=outline_rgb, B=(shape, "Outline"))
emissive = graph.op(unreal.MaterialExpressionAdd, -120, 0, A=outline_light, B=fill_light)
graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
graph.to_property((shape, "Disc"), unreal.MaterialProperty.MP_OPACITY)
graph.finish()

unreal.log(f"PULSECIRCLE::built {FOLDER}/M_PulseCircle")
