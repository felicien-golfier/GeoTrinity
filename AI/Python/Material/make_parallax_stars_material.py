"""Create M_ParallaxStars + MI_ParallaxStars_Far/Mid/Near — the backdrop layers MPC_Camera drives.

Orthographic projection has no perspective divide, so a backdrop plane gets no parallax from being
far away: one metre behind the floor and ten kilometres behind it translate on screen by exactly the
same amount, and AGeoGameCamera never rotates, so there is no sky-sphere spin either. All of the
depth is faked here, against the camera position AGeoGameCamera publishes into MPC_Camera every tick.

  UV = ( lerp(WorldXY, WorldXY - CameraXY, Parallax) * lerp(1, ZoomRatio, ZoomImmunity) ) / Tiling
       + Time * Drift

Parallax 0 welds the layer to the floor (it moves exactly like the ground), 1 pins it to the screen —
an infinitely distant sky. ZoomImmunity is the other half of that illusion: the camera widens
OrthoWidth from BaseOrthoWidth to MaxOrthoWidth for couch coop, which shrinks a world-locked layer's
features on screen and reads as the background rushing at the players. A genuinely distant layer must
not change screen size at all, so it scales its UVs by ZoomRatio to cancel the widening exactly.
Drift is ambient motion that survives a camera standing still — the boss fights clamp the camera to
an AGeoCameraVolume, which is precisely when parallax has the least to work with.

THE PER-LAYER KNOBS LIVE HERE, NOT IN THE MPC. A collection holds one value per parameter for the
whole world, so three layers sharing one Parallax is no layering at all. The collection carries only
what is genuinely global and per-frame (CameraXY, ZoomRatio); everything that differs per layer is a
material parameter, and the three instances at the bottom are the only place they differ.

WORLD POSITION, NEVER TexCoord. The backdrop planes will carry arbitrary scales and rotations like
the floor pieces do, so a UV-space starfield would run at a different density on every plane and
break at each seam. Absolute world position also puts the layer in the same space as CameraXY, so
neither side needs a transform. Same reasoning as make_background_lattice_material.py.

Stars are hashed per cell, not sampled from a texture: floor(UV) names the cell, three sin-hashes of
that cell give the star its position inside the cell and its keep/twinkle roll, and Density is a Step
against the third hash — so density, size and tiling are all continuous knobs on the instance instead
of three authored textures. Cost is a handful of ALU per pixel on a full-screen layer, which beats
the bandwidth of the tiling texture it replaces.

Additive and Unlit: the layers stack over each other and over whatever is behind the floor, and black
between the stars has to cost nothing. That also keeps them out of custom depth, so
MI_DeployableOutline never outlines the sky.

A star is a small bright core plus a thin four-armed cross, both fading out smoothly, and both scaled
by the same per-star size roll — the cross is what reads as "shiny" rather than "a dot". The star
shines on its own timer: ShineCycle is the period, ShineCycleRandom spreads it per star so the sky
never pulses in lockstep, and ShineDuration/ShineFalloff shape the flash inside that period.

THE CENTRE IS INSET TO 0.2..0.8 OF ITS CELL. Each cell only ever draws its own star, so anything
reaching past the cell border is cut off there — a cross whose arms are longer than the margin would
end in a straight edge instead of a fade. Insetting leaves 0.2 of clearance on every side, which is
what makes CrossLength safe to raise to 0.15 without a nine-cell neighbour loop.

The hash takes frac() BEFORE the sine, not after only. Cell indices run into the thousands out in the
level, and sin() of a thousand times 43758 is past what a float carries — the rolls stop varying per
cell and start banding into large smooth regions, which draws one enormous blob per band rather than
one star per cell. Wrapping the dot product into 0..1 first keeps every later step in range.

Rebuilt in place, never deleted and recreated — the backdrop planes reference the instances, and a
delete on a referenced asset opens a modal nobody is there to answer. The LAYERS table below only
seeds an instance the first time it is created — every layer is tuned on its instance in the editor,
and a rerun rebuilds the graph without touching those values.

Run outside PIE — creating expressions fails while a session is running.
"""
import unreal

PKG_PATH = "/Game/VFX/Generic/Materials"
NAME = "M_ParallaxStars"
MPC_PATH = f"{PKG_PATH}/MPC_Camera"
CAMERA_XY_PARAM = "CameraXY"
ZOOM_RATIO_PARAM = "ZoomRatio"

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
ALWAYS = unreal.PropertyAccessChangeNotifyMode.ALWAYS

# Tiling is the world size of one cell, so it is also the star spacing; Density is the fraction of
# cells that keep their star. Farther layers are denser, smaller and dimmer.
LAYERS = (
    ("MI_ParallaxStars_Far", {
        "Parallax": 0.90, "ZoomImmunity": 1.0, "Tiling": 220.0, "Density": 0.55,
        "StarRadius": 0.010, "CrossLength": 0.07, "Brightness": 2.0, "ShineCycle": 7.0,
    }, (0.0, 0.0), unreal.LinearColor(0.65, 0.75, 1.0, 1.0)),
    ("MI_ParallaxStars_Mid", {
        "Parallax": 0.70, "ZoomImmunity": 0.6, "Tiling": 450.0, "Density": 0.35,
        "StarRadius": 0.013, "CrossLength": 0.10, "Brightness": 3.5, "ShineCycle": 5.0,
    }, (0.0020, 0.0010), unreal.LinearColor(1.0, 0.95, 0.85, 1.0)),
    ("MI_ParallaxStars_Near", {
        "Parallax": 0.55, "ZoomImmunity": 0.2, "Tiling": 900.0, "Density": 0.20,
        "StarRadius": 0.018, "CrossLength": 0.14, "Brightness": 5.0, "ShineCycle": 3.5,
    }, (0.0060, 0.0030), unreal.LinearColor(0.9, 0.85, 1.0, 1.0)),
)


def save(asset):
    """Write asset to disk, or fail loudly.

    save_asset defaults to only_if_is_dirty and reports the outcome only through a return value, so a
    freshly created asset whose package never got flagged dirty writes NOTHING and says nothing.
    """
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), \
        f"{asset.get_path_name()}: would not save"


# load_asset, not does_asset_exist: the latter answers from the asset registry, which reports False
# for a collection saved earlier in the same editor session until it rescans.
collection = unreal.EditorAssetLibrary.load_asset(MPC_PATH)
assert collection, f"{MPC_PATH}: missing — run make_camera_mpc.py first"

mat = unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{NAME}")
if mat:
    # DeleteAllMaterialExpressions range-iterates the very array each delete removes from, so one
    # call drops only every other node. Repeat until it is really empty.
    while mel.get_num_material_expressions(mat):
        before = mel.get_num_material_expressions(mat)
        mel.delete_all_material_expressions(mat)
        assert mel.get_num_material_expressions(mat) < before, "material graph would not clear"
else:
    mat = at.create_asset(NAME, PKG_PATH, unreal.Material, unreal.MaterialFactoryNew())


def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def connect(source, target, target_input):
    # A wrong pin name is not an error, it is a no-op that leaves the input on its default — and a
    # defaulted input still compiles, into a layer that is simply the wrong distance away.
    assert mel.connect_material_expressions(source, "", target, target_input), (
        f"{type(source).__name__} -> {type(target).__name__}.{target_input}")


def binary(cls, a, b, x, y, first="A", second="B"):
    node = expr(cls, x, y)
    connect(a, node, first)
    connect(b, node, second)
    return node


def unary(cls, source, x, y):
    node = expr(cls, x, y)
    connect(source, node, "")
    return node


def scalar(name, value, x, y, group="Layer"):
    node = expr(unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    node.set_editor_property("group", group)
    return node


def color(name, value, x, y, group="Layer"):
    node = expr(unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    node.set_editor_property("group", group)
    return node


def mask(source, x, y, r=False, g=False, b=False, a=False):
    node = expr(unreal.MaterialExpressionComponentMask, x, y)
    connect(source, node, "")
    node.set_editor_property("r", r)
    node.set_editor_property("g", g)
    node.set_editor_property("b", b)
    node.set_editor_property("a", a)
    return node


def const2(x_value, y_value, x, y):
    node = expr(unreal.MaterialExpressionConstant2Vector, x, y)
    node.set_editor_property("r", x_value)
    node.set_editor_property("g", y_value)
    return node


def times(a, b, x, y):
    return binary(unreal.MaterialExpressionMultiply, a, b, x, y)


def scaled(source, factor, x, y):
    node = expr(unreal.MaterialExpressionMultiply, x, y)
    connect(source, node, "A")
    node.set_editor_property("const_b", factor)
    return node


def collection_param(name, x, y):
    node = expr(unreal.MaterialExpressionCollectionParameter, x, y)
    node.set_editor_property("collection", collection, ALWAYS)
    # The node resolves its parameter id in the post-change notification; assigned quietly it stays
    # unresolved and compiles to zero — a backdrop that never moves, with nothing warning about it.
    node.set_editor_property("parameter_name", name, ALWAYS)
    assert node.get_editor_property("collection") == collection \
        and str(node.get_editor_property("parameter_name")) == name, \
        f"{name}: collection parameter node would not take its binding"
    return node


# --- UV: the whole illusion --------------------------------------------------------------------
world_position = expr(unreal.MaterialExpressionWorldPosition, -2800, -400)
p = mask(world_position, -2600, -400, r=True, g=True)

camera_xy = mask(collection_param(CAMERA_XY_PARAM, -2800, -240), -2600, -240, r=True, g=True)
zoom_ratio = collection_param(ZOOM_RATIO_PARAM, -2800, -80)

parallax = scalar("Parallax", 0.9, -2800, 80)
zoom_immunity = scalar("ZoomImmunity", 1.0, -2800, 240)
tiling = scalar("Tiling", 900.0, -2800, 400)
drift = mask(color("Drift", unreal.LinearColor(0.0, 0.0, 0.0, 0.0), -2800, 560),
             -2600, 560, r=True, g=True)

relative = binary(unreal.MaterialExpressionSubtract, p, camera_xy, -2400, -320)
# Parallax 0 keeps p (welded to the world), 1 takes p - CameraXY (pinned to the screen).
anchored = expr(unreal.MaterialExpressionLinearInterpolate, -2200, -320)
connect(p, anchored, "A")
connect(relative, anchored, "B")
connect(parallax, anchored, "Alpha")

# ZoomImmunity 0 leaves the layer to shrink with the zoom-out, 1 scales its UVs by ZoomRatio, which
# cancels the OrthoWidth widening exactly and holds the layer at a constant screen size.
zoom_scale = expr(unreal.MaterialExpressionLinearInterpolate, -2200, 160)
zoom_scale.set_editor_property("const_a", 1.0)
connect(zoom_ratio, zoom_scale, "B")
connect(zoom_immunity, zoom_scale, "Alpha")

zoomed = times(anchored, zoom_scale, -2000, -280)
tiled = binary(unreal.MaterialExpressionDivide, zoomed, tiling, -1840, -280)

time = expr(unreal.MaterialExpressionTime, -2400, 560)
uv = binary(unreal.MaterialExpressionAdd, tiled, times(time, drift, -2000, 560), -1680, -280)

# --- one star per cell, hashed from the cell index ----------------------------------------------
cell = unary(unreal.MaterialExpressionFloor, uv, -1500, -400)
local = unary(unreal.MaterialExpressionFrac, uv, -1500, -160)


def shifted(source, offset_value, x, y):
    node = expr(unreal.MaterialExpressionAdd, x, y)
    connect(source, node, "A")
    node.set_editor_property("const_b", offset_value)
    return node


def saturated(source, x, y):
    return unary(unreal.MaterialExpressionSaturate, source, x, y)


def fade(value_node, extent, x, y):
    """saturate(1 - value/extent) — 1 at the centre, reaching 0 exactly at extent."""
    return saturated(unary(unreal.MaterialExpressionOneMinus,
                           binary(unreal.MaterialExpressionDivide, value_node, extent, x, y),
                           x + 160, y),
                     x + 320, y)


def sharpen(source, exponent, x, y):
    return binary(unreal.MaterialExpressionPower, source, exponent, x, y,
                  first="Base", second="Exp")


def hash_of(normal_x, normal_y, factor, x, y):
    """frac(sin(frac(dot(cell, n))) * k) — one deterministic 0..1 roll per cell.

    The inner frac is what keeps this working away from the origin: cell indices reach the thousands,
    and sin() of a number that large multiplied by k lands outside float precision, so the rolls stop
    varying per cell and band into large smooth regions.
    """
    projected = binary(unreal.MaterialExpressionDotProduct, cell,
                       const2(normal_x, normal_y, x - 200, y + 80), x, y)
    wrapped = unary(unreal.MaterialExpressionFrac, projected, x + 160, y)
    sine = unary(unreal.MaterialExpressionSine, wrapped, x + 320, y)
    sine.set_editor_property("period", 1.0)
    return unary(unreal.MaterialExpressionFrac, scaled(sine, factor, x + 480, y), x + 640, y)


hash_x = hash_of(12.9898, 78.2330, 43758.5453, -1300, -1100)
hash_y = hash_of(39.3468, 11.1350, 23421.6312, -1300, -940)
roll = hash_of(63.7264, 27.9182, 19349.1234, -1300, -780)
hash_size = hash_of(21.4413, 55.6172, 31627.8391, -1300, -620)
hash_bright = hash_of(74.8112, 33.2907, 27183.4519, -1300, -460)
hash_phase = hash_of(47.1553, 91.7264, 15731.2748, -1300, -300)

# Inset to 0.2..0.8 so the cross arms stay clear of the cell border, which would otherwise cut them
# off in a straight line instead of letting them fade.
star_centre = binary(unreal.MaterialExpressionAppendVector,
                     shifted(scaled(hash_x, 0.6, -500, -1100), 0.2, -340, -1100),
                     shifted(scaled(hash_y, 0.6, -500, -940), 0.2, -340, -940), -180, -1040)
offset = binary(unreal.MaterialExpressionSubtract, local, star_centre, -20, -1000)
distance = unary(unreal.MaterialExpressionLength, offset, 140, -1000)
distance_x = unary(unreal.MaterialExpressionAbs, mask(offset, 140, -880, r=True), 300, -880)
distance_y = unary(unreal.MaterialExpressionAbs, mask(offset, 140, -760, g=True), 300, -760)

star_radius = scalar("StarRadius", 0.012, -2800, 720)
sharpness = scalar("Sharpness", 1.5, -2800, 880)
density = scalar("Density", 0.35, -2800, 1040)
size_random = scalar("SizeRandom", 0.6, -2800, 1200)
cross_length = scalar("CrossLength", 0.10, -2800, 1360)
cross_width = scalar("CrossWidth", 0.004, -2800, 1520)
cross_falloff = scalar("CrossFalloff", 1.5, -2800, 1680)
cross_brightness = scalar("CrossBrightness", 0.35, -2800, 1840)

# 1 - SizeRandom*(1 - roll): SizeRandom 0 makes every star the authored size, 1 lets the smallest
# shrink to nothing. Core and cross share it, so a big star is big in both.
size_scale = unary(unreal.MaterialExpressionOneMinus,
                   times(size_random, unary(unreal.MaterialExpressionOneMinus, hash_size,
                                            -500, -620), -340, -620),
                   -180, -620)

core = sharpen(fade(distance, times(star_radius, size_scale, 300, -1000), 460, -1000),
               sharpness, 940, -1000)

# Each arm is a long thin ridge: it fades along its own length and across its width, and the two
# fades multiplied are what taper the arm to a point rather than ending it in a blunt line.
arm_length = times(cross_length, size_scale, 300, -640)
arm_x = sharpen(times(fade(distance_x, arm_length, 460, -880),
                      fade(distance_y, cross_width, 460, -520), 940, -880),
                cross_falloff, 1100, -880)
arm_y = sharpen(times(fade(distance_y, arm_length, 460, -400),
                      fade(distance_x, cross_width, 460, -280), 940, -400),
                cross_falloff, 1100, -400)
cross = times(binary(unreal.MaterialExpressionAdd, arm_x, arm_y, 1260, -640),
              cross_brightness, 1420, -640)

# Step(Y, X) is 1 where X >= Y — the cell keeps its star only when its roll falls under Density.
keep = binary(unreal.MaterialExpressionStep, roll, density, 300, -160, first="Y", second="X")

# --- shine -----------------------------------------------------------------------------------------
# Every star runs its own clock: ShineCycle is the period, ShineCycleRandom spreads it either side so
# no two neighbours share one, and hash_phase offsets where in that period each star starts. Inside
# the period the flash is a single decay over ShineDuration, so a star sits dim and then catches.
shine_cycle = scalar("ShineCycle", 5.0, -2800, 2000)
shine_cycle_random = scalar("ShineCycleRandom", 0.8, -2800, 2160)
shine_duration = scalar("ShineDuration", 0.15, -2800, 2320)
shine_falloff = scalar("ShineFalloff", 3.0, -2800, 2480)
shine_intensity = scalar("ShineIntensity", 3.0, -2800, 2640)

cycle_spread = expr(unreal.MaterialExpressionLinearInterpolate, -500, 2160)
connect(unary(unreal.MaterialExpressionOneMinus, shine_cycle_random, -900, 2160), cycle_spread, "A")
connect(shifted(shine_cycle_random, 1.0, -900, 2280), cycle_spread, "B")
connect(hash_phase, cycle_spread, "Alpha")
cycle = times(shine_cycle, cycle_spread, -340, 2100)

phase = unary(unreal.MaterialExpressionFrac,
              binary(unreal.MaterialExpressionAdd,
                     binary(unreal.MaterialExpressionDivide, time, cycle, -180, 2100),
                     hash_phase, -20, 2100),
              140, 2100)
flash = sharpen(fade(phase, shine_duration, 300, 2100), shine_falloff, 940, 2100)
shine = shifted(times(shine_intensity, flash, 1100, 2100), 1.0, 1260, 2100)

# --- output ---------------------------------------------------------------------------------------
brightness = scalar("Brightness", 3.5, -2800, 2800)
brightness_random = scalar("BrightnessRandom", 0.7, -2800, 2960)
star_color = color("StarColor", unreal.LinearColor(0.8, 0.85, 1.0, 1.0), -2800, 3120)

star_brightness = times(brightness,
                        unary(unreal.MaterialExpressionOneMinus,
                              times(brightness_random,
                                    unary(unreal.MaterialExpressionOneMinus, hash_bright,
                                          -500, -460), -340, -460),
                              -180, -460), 1600, 2800)

shape = times(binary(unreal.MaterialExpressionAdd, core, cross, 1600, -800), keep, 1760, -800)
value = times(times(shape, shine, 1920, -800), star_brightness, 2080, -800)
emissive = times(star_color, value, 2240, -800)

mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(saturated(value, 2240, -600), "", unreal.MaterialProperty.MP_OPACITY)

# --- settings (after wiring) ---------------------------------------------------------------------
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)

mel.recompile_material(mat)
save(mat)

# --- the three layers -----------------------------------------------------------------------------
# Written only when the instance is created. These are the tuning knobs, and tuning happens in the
# editor on the instance — a rerun rebuilding the graph must never throw that away.
for name, values, layer_drift, tint in LAYERS:
    if unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{name}"):
        continue
    instance = at.create_asset(name, PKG_PATH, unreal.MaterialInstanceConstant,
                               unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, mat)
    for param, value_of in values.items():
        mel.set_material_instance_scalar_parameter_value(instance, param, value_of)
        # Approximate: the value comes back as the float32 the instance stores, never the exact double.
        assert abs(mel.get_material_instance_scalar_parameter_value(instance, param)
                   - value_of) < 1e-4, f"{name}: {param} would not take"
    mel.set_material_instance_vector_parameter_value(
        instance, "Drift", unreal.LinearColor(layer_drift[0], layer_drift[1], 0.0, 0.0))
    mel.set_material_instance_vector_parameter_value(instance, "StarColor", tint)
    save(instance)

unreal.log(f"STARS::built {PKG_PATH}/{NAME} + {len(LAYERS)} instances")
