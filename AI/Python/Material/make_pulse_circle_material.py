"""Create M_PulseCircle — an unlit additive circle: constant outline ring, pulsing inside.

Per pixel:
  uvn     = (TexCoord - 0.5) * 2          -> quad space, [-1,1] on both axes
  dist    = length(uvn)                   -> 0 at center, 1 on the circle, >1 in the quad corners
  inner   = 1 - OutlineThickness

  Disc    = Step(dist, 1)                 -> the whole circle
  Fill    = Step(dist, inner)             -> interior, stopping at the ring's inner edge
  Outline = Disc - Fill                   -> the ring alone; the two never overlap

  Offset  = frac(dot(ObjectPositionWS, (0.13731, 0.27113, 0.05777)))
  Pulse   = sin(dist + Time * PulseSpeed + Offset) * 0.35 + 0.65
  Wipe    = MF_DurationWipe(TexCoord, DurationSpent)
  Emissive = OutlineColor * Outline + InsideColor * Fill * Pulse * Wipe
  Opacity  = Disc

The pulse travels INWARD because dist and time enter the sine with the same sign: a crest sits
where dist + Time*PulseSpeed is constant, so its radius shrinks as time advances. Sine's Period
is left at 1, which makes exactly one cycle span dist 0..1 — the phase at the center and at the
rim therefore match, and the band flows in continuously with no seam where it wraps.

The 0.35/0.65 remap keeps the pulse in [0.30, 1.00] instead of [0, 1]: an additive material at 0
is invisible, so a full-depth wave would blink the inside out entirely once per cycle rather than
breathe. Opacity is the bare Disc and deliberately does NOT carry Pulse — additive already
multiplies emissive by opacity, so feeding the pulse into both would square it.

Offset is what stops every zone in the level pulsing on the same beat, and it costs no code and no
MID: the object's own world position hashes to a stable 0..1, and since Sine's Period is 1 that
range spreads zones across a full cycle. PerInstanceRandom cannot do this job here — RandomID is
only ever filled in by HierarchicalInstancedStaticMesh, so on the plain UStaticMeshComponent a
deployable carries it reads 0 for every zone and they all stay in lockstep. The origin is pinned to
Absolute: Camera Relative is translated world space, which would slide the phase as the camera
moves. The flip side of hashing position is that a zone which MOVES re-hashes as it goes, so its
pulse warps mid-flight — fine for deployed zones that sit still, wrong for anything that travels.

The wipe eats the FILL only, never the ring: the outline stays the zone's full extent, which is the
part that carries gameplay meaning (how far the heal reaches), while the interior wedge reports how
much life is left. Outline is derived from the un-wiped Fill, so the ring keeps its width as the
wedge retreats.

DurationSpent reads custom primitive data slot 0 rather than a material instance value, so a whole
level of zones shares one material with no dynamic instance per actor: AGeoDeployableBase::Tick
writes the slot straight onto the mesh component. It is the fraction CONSUMED and defaults to 0 —
see MF_DurationWipe for why the inversion is load-bearing rather than cosmetic. Nothing else in the
project claims a custom-data slot, so 0 is free.

PulseSpeed is in sweeps per second. OutlineThickness is a fraction of the radius, so the ring
keeps its relative width at any scale.

Two-sided and Niagara-sprite enabled to match its neighbours M_ZoneIndicator / M_ZoneIndicatorRay,
so it drops onto a plane mesh or a sprite emitter without a usage-flag recompile.

Also creates MI_PulseCircle: use THAT one in-game. A base material's parameter defaults cannot be
tuned from the Details panel, an instance's can, and a MID for runtime animation parents off it.
"""
import unreal

PKG_PATH = "/Game/VFX/Generic/Materials"
NAME = "M_PulseCircle"
INSTANCE_NAME = "MI_PulseCircle"
WIPE_FUNCTION = "/Game/VFX/Generic/Materials/Functions/MF_DurationWipe"
FULL = f"{PKG_PATH}/{NAME}.{NAME}"

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

# Rebuilt in place, never deleted and recreated: anything already referencing the material would
# turn a delete into a "still referenced / force delete" modal, which blocks the game thread with
# nobody to answer it.
if unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{NAME}"):
    mat = unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{NAME}")
    # UMaterialEditingLibrary::DeleteAllMaterialExpressions range-iterates the very array each
    # delete removes from, so one call drops only every other node. Repeat until it is really empty.
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
    # defaulted Step input still compiles, into a circle that is simply the wrong size. Never call
    # the library's connect directly.
    assert mel.connect_material_expressions(source, "", target, target_input), (
        f"{type(source).__name__} -> {type(target).__name__}.{target_input}")


def scalar(name, value, x, y):
    node = expr(unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def color(name, value, x, y):
    node = expr(unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def multiply(a, b_node, b_const, x, y):
    node = expr(unreal.MaterialExpressionMultiply, x, y)
    connect(a, node, "A")
    if b_node:
        connect(b_node, node, "B")
    else:
        node.set_editor_property("const_b", b_const)
    return node


# --- dist: distance from the quad center, 1 on the circle ---
tex = expr(unreal.MaterialExpressionTextureCoordinate, -1500, -100)

centered = expr(unreal.MaterialExpressionSubtract, -1300, -100)
centered.set_editor_property("const_b", 0.5)
connect(tex, centered, "A")

uvn = multiply(centered, None, 2.0, -1120, -100)

dist = expr(unreal.MaterialExpressionLength, -940, -100)
connect(uvn, dist, "")

# --- the two rings of the shape ---
thickness = scalar("OutlineThickness", 0.08, -1300, 120)

inner = expr(unreal.MaterialExpressionOneMinus, -1080, 120)
connect(thickness, inner, "")

# Step(Y, X) is 1 where X >= Y. ConstX already defaults to 1, but the whole shape hangs off that
# radius, so it is pinned here rather than inherited.
disc = expr(unreal.MaterialExpressionStep, -700, -260)
disc.set_editor_property("const_x", 1.0)
connect(dist, disc, "Y")

fill = expr(unreal.MaterialExpressionStep, -700, 20)
connect(dist, fill, "Y")
connect(inner, fill, "X")

outline = expr(unreal.MaterialExpressionSubtract, -480, -180)
connect(disc, outline, "A")
connect(fill, outline, "B")

# --- per-zone phase offset: frac(dot(ObjectPositionWS, hash)) ---
object_pos = expr(unreal.MaterialExpressionObjectPositionWS, -1500, 760)
object_pos.set_editor_property("origin_type", unreal.PositionOrigin.ABSOLUTE)

# Scales chosen so 1 unit of movement shifts the phase by ~0.14-0.27 of a cycle: small coefficients
# make this a gradient rather than a hash, and neighbouring zones then land on nearly the same
# offset (the first pass used 0.0173/0.0311/0.0077 and put two zones 400 units apart just 0.064 of
# a cycle apart — a 64ms lag, indistinguishable from no offset at all). The long fractional parts
# matter just as much: round scales like 0.173 map grid-snapped placements onto a short lattice and
# collide EXACTLY, which is the normal case in a level editor, not a corner case.
hash_scale = expr(unreal.MaterialExpressionConstant3Vector, -1500, 920)
hash_scale.set_editor_property("constant", unreal.LinearColor(0.13731, 0.27113, 0.05777, 0.0))

pos_hash = expr(unreal.MaterialExpressionDotProduct, -1280, 820)
connect(object_pos, pos_hash, "A")
connect(hash_scale, pos_hash, "B")

offset = expr(unreal.MaterialExpressionFrac, -1080, 820)
connect(pos_hash, offset, "")

# --- Pulse = sin(dist + Time * PulseSpeed + Offset) * 0.35 + 0.65 ---
time = expr(unreal.MaterialExpressionTime, -1300, 420)
speed = scalar("PulseSpeed", 1.0, -1300, 580)

offset_time = expr(unreal.MaterialExpressionAdd, -880, 560)
connect(multiply(time, speed, None, -1080, 480), offset_time, "A")
connect(offset, offset_time, "B")

phase = expr(unreal.MaterialExpressionAdd, -700, 400)
connect(dist, phase, "A")
connect(offset_time, phase, "B")

wave = expr(unreal.MaterialExpressionSine, -540, 400)
connect(phase, wave, "")

pulse = expr(unreal.MaterialExpressionAdd, -220, 400)
pulse.set_editor_property("const_b", 0.65)
connect(multiply(wave, None, 0.35, -380, 400), pulse, "A")

# --- Wipe = MF_DurationWipe(TexCoord, DurationSpent) ---
spent = scalar("DurationSpent", 0.0, -1500, 1120)
spent.set_editor_property("use_custom_primitive_data", True)
spent.set_editor_property("primitive_data_index", 0)

wipe = expr(unreal.MaterialExpressionMaterialFunctionCall, -1080, 1060)
# Assigning the function is what builds the call's pins, and only a notify naming MaterialFunction
# reaches the handler that does it — a silent assignment leaves a node with no UV or Spent to
# connect to.
wipe.set_editor_property("material_function", unreal.EditorAssetLibrary.load_asset(WIPE_FUNCTION),
                         unreal.PropertyAccessChangeNotifyMode.ALWAYS)
connect(tex, wipe, "UV")
connect(spent, wipe, "Spent")

# --- Emissive = OutlineColor * Outline + InsideColor * Fill * Pulse * Wipe ---
outline_color = color("OutlineColor", unreal.LinearColor(1.0, 1.0, 1.0, 1.0), -160, -440)
inside_color = color("InsideColor", unreal.LinearColor(0.15, 0.6, 1.0, 1.0), -160, 640)

alive_fill = multiply(multiply(fill, pulse, None, -60, 120), wipe, None, 120, 160)

emissive = expr(unreal.MaterialExpressionAdd, 560, -80)
connect(multiply(outline_color, outline, None, 320, -320), emissive, "A")
connect(multiply(inside_color, alive_fill, None, 320, 220), emissive, "B")

mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(disc, "", unreal.MaterialProperty.MP_OPACITY)

# --- settings (after wiring) ---
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
mat.set_editor_property("used_with_niagara_sprites", True)

mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(f"{PKG_PATH}/{NAME}")

# Left alone on a rerun so tuned values live.
if not unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{INSTANCE_NAME}"):
    instance = at.create_asset(INSTANCE_NAME, PKG_PATH, unreal.MaterialInstanceConstant,
                               unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, mat)
    unreal.EditorAssetLibrary.save_asset(f"{PKG_PATH}/{INSTANCE_NAME}")

unreal.log(f"PULSECIRCLE::built {FULL} + {INSTANCE_NAME}")
