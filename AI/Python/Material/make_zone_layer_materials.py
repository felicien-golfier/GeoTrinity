"""The zone layer (World/ZoneLayer/CLAUDE.md, AI/Materials.md "Zone layer"): M_ZoneLayer, its two instances
MI_ZoneLayer_Ground and MI_ZoneLayer_Telegraph, and the functions it calls. Every zone, beam and telegraph is a slot of
the slot texture UGeoZoneLayerSubsystem writes; one HLSL loop (zone_layer_hlsl.py) turns the slots covering a pixel
into their colours, fill shade and outlines.

MF_SafeSpotCut cuts the safe spots of MPC_MaskedArea (FSafeSpotMask) out of the slots flagged SafeSpotCut; the script
adds the collection's SafeSpot_ShadowMode and SafeSpot_LaneDirection parameters when they are missing.

Run make_generic_material_functions.py and make_color_pattern_functions.py first. The ZoneSlots, ZoneCount and Palette
parameters are written by AGeoZoneLayerActor (GeoMaterialParams in GeoNiagaraParams.h): renaming one here means
renaming it there. Rebuilt in place, so the instances and every reference keep them."""
import unreal

FOLDER = "/Game/Art/VFX/ZoneLayer"
FUNCTIONS = FOLDER + "/Functions"
SAFE_SPOT_FUNCTIONS = "/Game/Art/VFX/AOE/Functions"
GENERIC = "/Game/Art/VFX/Generic/Materials/Functions"
CATEGORY = "GeoTrinity|ZoneLayer"
MASK_COLLECTION = "/Game/Art/VFX/Abilities/Enemies/PillarsAOE/MPC_MaskedArea"
MASK_SLOTS = 8
# Linear default for the texture parameters, the one the deployable outline's palette uses: C++ sets the real ones.
DEFAULT_TEXTURE = "/Engine/EngineMaterials/DefaultBloomKernel"
INSTANCES = {  # name: (draws telegraphs, fill opacity, fill brightness)
    "MI_ZoneLayer_Ground": (False, 0.6, 0.65),
    "MI_ZoneLayer_Telegraph": (True, 0.5, 1.0),
}
SINGLE_COLOR_SHADE = 0.35  # a lone telegraph colour's second stripe, see MF_TelegraphColors

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)
hlsl_path = unreal.Paths.project_dir() + "AI/Python/Material/zone_layer_hlsl.py"
hlsl = {}
exec(compile(open(hlsl_path).read(), hlsl_path, "exec"), hlsl)
load = toolkit["load"]
open_function = toolkit["open_function"]
mel = toolkit["mel"]
is_whole_number = load(GENERIC + "/MF_IsWholeNumber")
meaning_colors = load(GENERIC + "/MF_MeaningColors")
telegraph_colors = load(SAFE_SPOT_FUNCTIONS + "/MF_TelegraphColors")
pattern_collection = load("/Game/Art/VFX/Generic/Materials/MPC_MeaningColors")
mask_collection = load(MASK_COLLECTION)
# The editor asset library loads project content only.
default_texture = unreal.load_asset(DEFAULT_TEXTURE)
assert default_texture, f"{DEFAULT_TEXTURE}: missing"
S = unreal.MaterialExpressionScalarParameter
FLOAT1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1
FLOAT3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
FLOAT4 = unreal.CustomMaterialOutputType.CMOT_FLOAT4

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
graph = open_function(SAFE_SPOT_FUNCTIONS, "MF_SafeSpotSlotCut", "1 where one safe spot shadows a world position: its "
                      "whole strip across the spot when IsLane, its round hole when IsDisc, nothing for an unused "
                      "slot.", CATEGORY)
world_xy = graph.input("WorldXY", "Vector2", 0, "The pixel's world position, cm.")
spot = graph.input("Spot", "Vector4", 1, "The spot's world XY in red and green, alpha 1 when the slot is used.")
radius = graph.input("Radius", "Scalar", 2, "Radius of the hole, or half width of the strip, cm.")
is_lane = graph.input("IsLane", "Scalar", 3, "1 when the spot cuts a strip across the shape.")
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
graph = open_function(SAFE_SPOT_FUNCTIONS, "MF_SafeSpotCut", "1 where one of the safe spots of MPC_MaskedArea shadows "
                      "a world position: Lane (mode 1) cuts the whole strip across the spot, Disc (mode 2) its round "
                      "hole. Behind (mode 0) and unused slots cut nothing.", CATEGORY)
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

# --- MF_ZoneShapeDistance: one shape's signed distance, for any material drawing a zone shape ---
graph = open_function(FUNCTIONS, "MF_ZoneShapeDistance", "Signed distance from a world position to one zone shape's "
                      "outline, negative inside, with its core and inner extent: the shape's slot as "
                      "UGeoZoneLayerSubsystem writes it.", CATEGORY)
shape = graph.input("Shape", "Scalar", 0, "EGeoZoneShape ordinal: 0 circle, 1 beam.")
position = graph.input("Position", "Vector2", 1, "World XY, cm.")
params0 = graph.input("Params0", "Vector4", 2, "Slot row 1: circle centre XY and radius; beam start XY, end XY.")
params1 = graph.input("Params1", "Vector4", 3, "Slot row 2: beam half-width in red.")
distance = graph.custom("Zone shape distance", hlsl["SHAPE_DISTANCE"], FLOAT1,
                        {"Shape": shape, "Position": position, "Params0": params0, "Params1": params1},
                        {"Extent": FLOAT1, "Core": FLOAT1})
graph.output("Distance", 0, "Signed distance to the outline, cm, negative inside.", (distance, "return"))
graph.output("Core", 1, "Distance from the shape's core: circle centre, beam middle line.", (distance, "Core"))
graph.output("Extent", 2, "The core's distance to the outline: radius, half-width.", (distance, "Extent"))
graph.finish()

# --- MF_ZoneLayerCoverage: every slot of one layer covering the pixel ---
graph = open_function(FUNCTIONS, "MF_ZoneLayerCoverage", "Loops the ZoneCount slots of the zone layer's slot texture "
                      "for the slots of one layer covering a world position: the union of their colours, how many "
                      "carry each of the first four, the fill shade and the outline.", CATEGORY)
slots = graph.input("ZoneSlots", "Texture2D", 0, "The slot texture UGeoZoneLayerSubsystem writes.")
count = graph.input("ZoneCount", "Scalar", 1, "How many slots are live.")
world_xy = graph.input("WorldXY", "Vector2", 2, "The pixel's world position, cm.")
is_telegraph = graph.input("IsTelegraph", "Scalar", 3, "1 to draw the Telegraph slots, 0 the Ground ones.")
cut_mask = graph.input("CutMask", "Scalar", 4, "1 where a safe spot shadows the pixel: slots flagged SafeSpotCut "
                       "cover nothing there.")
outline_width = graph.input("OutlineWidth", "Scalar", 5, "Width of every shape's outline, cm.")
coverage = graph.custom("Zone layer coverage", hlsl["COVERAGE"], FLOAT1,
                        {"ZoneSlots": slots, "ZoneCount": count, "WorldXY": world_xy, "IsTelegraph": is_telegraph,
                         "CutMask": cut_mask, "OutlineWidth": outline_width},
                        {"Counts": FLOAT4, "Covered": FLOAT1, "Shade": FLOAT1, "Outline": FLOAT1,
                         "OutlineColorIndex": FLOAT1})
graph.output("ColorMask", 0, "Union of the covering slots' colour bits.", (coverage, "return"))
graph.output("Counts", 1, "How many covering slots carry each of the first four colour bits, in ordinal order.",
             (coverage, "Counts"))
graph.output("Covered", 2, "1 where a slot covers the pixel.", (coverage, "Covered"))
graph.output("Shade", 3, "1 where a covering slot's Fill has grown from its core.", (coverage, "Shade"))
graph.output("Outline", 4, "1 on a covering slot's outline.", (coverage, "Outline"))
graph.output("OutlineColorIndex", 5, "Palette slot of that outline: its slot's first colour.",
             (coverage, "OutlineColorIndex"))
graph.finish()
layer_coverage = graph.asset

# --- MF_ZoneMaskColors: a colour mask's first four colours, each brightened by how many shapes stack it ---
graph = open_function(FUNCTIONS, "MF_ZoneMaskColors", "The first four colours of a colour mask, in ordinal order, read "
                      "from the palette texture, each brightened by how many shapes carry it (MPC_MeaningColors' "
                      "StackBrightness, up to StackBrightnessMax); and an outline's colour.", CATEGORY)
color_mask = graph.input("ColorMask", "Scalar", 0, "One bit per EGeoColor ordinal.")
counts = graph.input("Counts", "Vector4", 1, "How many shapes carry each of the first four colours.")
outline_index = graph.input("OutlineColorIndex", "Scalar", 2, "Palette slot of the outline.")
palette = graph.input("Palette", "Texture2D", 3, "The palette texture, one texel per EGeoColor ordinal.")
mask_colors = graph.custom("Zone mask colours", hlsl["MASK_COLORS"], FLOAT1,
                           {"ColorMask": color_mask, "Counts": counts, "OutlineColorIndex": outline_index,
                            "Palette": palette,
                            "StackBrightness": graph.collection_parameter(pattern_collection, "StackBrightness"),
                            "StackBrightnessMax": graph.collection_parameter(pattern_collection,
                                                                             "StackBrightnessMax")},
                           {"Color0": FLOAT3, "Color1": FLOAT3, "Color2": FLOAT3, "Color3": FLOAT3,
                            "OutlineColor": FLOAT3})
graph.output("ColorCount", 0, "How many colours the mask carries, 1 to 4.", (mask_colors, "return"))
for slot in range(4):
    graph.output(f"Color{slot}", 1 + slot, f"Colour of meaning {slot + 1}.", (mask_colors, f"Color{slot}"))

graph.output("OutlineColor", 5, "The outline's colour.", (mask_colors, "OutlineColor"))
graph.finish()
zone_mask_colors = graph.asset


def telegraph_switch(graph, telegraph, ground):
    """The Telegraph static switch: telegraph on the telegraph plane, ground on the ground plane."""
    switch = graph.node(unreal.MaterialExpressionStaticSwitchParameter, parameter_name="Telegraph",
                        default_value=False, group="01 Layer", sort_priority=0,
                        desc="On: this plane draws the Telegraph slots, in stripes; off: the Ground slots.")
    graph.connect(telegraph, switch, "True")
    graph.connect(ground, switch, "False")
    return switch


def texture_parameter(graph, name, sort_priority, description):
    return graph.node(unreal.MaterialExpressionTextureObjectParameter, parameter_name=name, texture=default_texture,
                      sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, group="02 Slots",
                      sort_priority=sort_priority, desc=description)


# --- M_ZoneLayer: one plane of the zone layer ---
graph = toolkit["open_material"](FOLDER, "M_ZoneLayer")
world_xy = graph.mask(graph.node(unreal.MaterialExpressionWorldPosition), "rg")
slots = texture_parameter(graph, "ZoneSlots", 0, "Slot texture, written every tick by UGeoZoneLayerSubsystem.")
palette = texture_parameter(graph, "Palette", 1, "Palette texture, one texel per EGeoColor ordinal "
                            "(GeoColor::CreatePaletteTexture).")
count = graph.parameter(S, "ZoneCount", 0.0, "02 Slots", 2, "How many slots are live; AGeoZoneLayerActor writes it.")
is_telegraph = telegraph_switch(graph, graph.node(unreal.MaterialExpressionConstant, r=1.0),
                                graph.node(unreal.MaterialExpressionConstant, r=0.0))
cut = graph.call(safe_spot_cut, WorldXY=world_xy)
coverage = graph.call(layer_coverage, ZoneSlots=slots, ZoneCount=count, WorldXY=world_xy, IsTelegraph=is_telegraph,
                      CutMask=(cut, "CutMask"),
                      OutlineWidth=graph.collection_parameter(pattern_collection, "OutlineWidth"))
colors = graph.call(zone_mask_colors, ColorMask=(coverage, "ColorMask"), Counts=(coverage, "Counts"),
                    OutlineColorIndex=(coverage, "OutlineColorIndex"), Palette=palette)
meanings = {f"Color{slot}": (colors, f"Color{slot}") for slot in range(4)}
ground = graph.call(meaning_colors, ColorCount=(colors, "ColorCount"), **meanings)
shade = graph.parameter(S, "SingleColorShade", SINGLE_COLOR_SHADE, "03 Look", 0, "Telegraph only: brightness of a "
                        "lone colour's second stripe.")
telegraph = graph.call(telegraph_colors, ColorCount=(colors, "ColorCount"), SingleColorShade=shade, **meanings)
pattern = telegraph_switch(graph, (telegraph, "Color"), (ground, "Color"))

# Spent part darker, then each outline over everything.
darkening = graph.op(unreal.MaterialExpressionMultiply, A=(coverage, "Shade"),
                     B=graph.collection_parameter(pattern_collection, "FillShade"))
kept = graph.op(unreal.MaterialExpressionOneMinus, darkening)
shaded = graph.op(unreal.MaterialExpressionMultiply, A=pattern, B=kept)
brightness = graph.parameter(S, "FillBrightness", 1.0, "03 Look", 1, "Emissive multiplier of the fill.")
fill = graph.op(unreal.MaterialExpressionMultiply, A=shaded, B=brightness)
emissive = graph.op(unreal.MaterialExpressionLinearInterpolate, A=fill, B=(colors, "OutlineColor"),
                    Alpha=(coverage, "Outline"))
fill_opacity = graph.parameter(S, "FillOpacity", 0.5, "03 Look", 2, "Opacity of the fill, against the outline's 1.")
faint_fill = graph.op(unreal.MaterialExpressionMultiply, A=(coverage, "Covered"), B=fill_opacity)
opacity = graph.op(unreal.MaterialExpressionMax, A=faint_fill, B=(coverage, "Outline"))
graph.to_property(emissive, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
graph.to_property(opacity, unreal.MaterialProperty.MP_OPACITY)
graph.asset.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
graph.asset.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
graph.finish()
master = graph.asset

# --- MI_ZoneLayer_Ground, MI_ZoneLayer_Telegraph ---
for name, (draws_telegraphs, opacity_value, brightness_value) in INSTANCES.items():
    instance = toolkit["load_or_create"](FOLDER, name, unreal.MaterialInstanceConstant,
                                         unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, master)
    mel.set_material_instance_static_switch_parameter_value(instance, "Telegraph", draws_telegraphs)
    mel.set_material_instance_scalar_parameter_value(instance, "FillOpacity", opacity_value)
    mel.set_material_instance_scalar_parameter_value(instance, "FillBrightness", brightness_value)
    mel.update_material_instance(instance)
    toolkit["save"](instance)

unreal.log(f"ZONELAYER::built {FOLDER}/M_ZoneLayer, {', '.join(INSTANCES)}, {FUNCTIONS} and "
           f"{SAFE_SPOT_FUNCTIONS}/MF_SafeSpotCut")
