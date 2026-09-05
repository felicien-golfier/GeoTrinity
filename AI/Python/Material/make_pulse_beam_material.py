"""Create M_PulseBeam — the rectangular beam sibling of M_PulseCircle: outline frame, pulsing inside.

Same treatment as M_PulseCircle (unlit, additive, two-sided, hard-edged), on a beam quad instead of
a circle, and carrying the Moira beam's parameter set on top of the outline/pulse one.

  u, v    = TexCoord            u runs along the beam's LENGTH, v across its width
  du      = abs((u - 0.5) * 2)  0 mid-beam -> 1 at the two end caps
  dv      = abs((v - 0.5) * 2)  0 mid-beam -> 1 at the two side rails

  thickU  = OutlineThickness * (Beam_Width / Beam_Length)
  Outline = max( Step(1 - OutlineThickness, dv), Step(1 - thickU, du) )
  Fill    = 1 - Outline

  Pulse    = sin(u * PulseCount - Time * PulseSpeed) * 0.35 + 0.65
  Wipe     = Step(u, 1 - DurationSpent)
  Emissive = (OutlineColor * Outline + InsideColor * Fill * Pulse * Wipe) * Color

Beam_Width and Beam_Length are not carried just to mirror NS_Cirlce_MoiraBeam's user parameters —
the outline cannot be uniform without them. OutlineThickness is a fraction of the beam's WIDTH, so
across the width it applies directly, but the same world thickness along a 10x longer beam is only
a tenth as much UV; without the Width/Length correction an 8% frame on a 1000x100 beam draws end
caps ten times thicker than the side rails.

Names match the Niagara user parameters (Beam_Length, Beam_Width, Color) so the mapping is obvious,
but nothing binds them automatically: a Niagara User parameter does not reach a material parameter
on its own, that link is authored in the emitter. Color is a master tint applied last and defaults
to white, so it is a no-op until something drives it — the authored look lives in OutlineColor and
InsideColor, and Color stays free for the per-cast tint UGeoBeamVFXComponent::BeamColor pushes.

The pulse runs from caster to target because u and Time enter the sine with OPPOSITE signs — a crest
sits where u*PulseCount - Time*PulseSpeed is constant, so its u grows as time advances. That is the
mirror of M_PulseCircle, where the two share a sign and the band travels inward instead.

DurationSpent runs the same remaining-life readout M_PulseCircle carries, along the beam's LENGTH
rather than around a disc, so the lit span retracts from the target back toward the caster. It stays
a plain scalar parameter instead of reading custom primitive data the way the circle's copy does:
this material is driven from a Niagara emitter, and a renderer binds material PARAMETERS — custom
primitive data belongs to a primitive component, which a sprite is not. A parameter also carries a
real default, so the 0-means-untouched inversion the circle needs for safety is only a naming
convention here; it is kept anyway so both materials read the same way.

It does not call MF_DurationWipe. That function's cost is the atan2 chain turning UV into an angle,
and a wipe along one axis already has its coordinate in hand — u IS the progress. Routing it through
the function would mean converting a straight line into an angle and back.

Like the circle, the wipe eats the FILL only. The outline keeps the beam's full reach, so the frame
reports where the beam still lands while the interior reports how much is left. Wiping the outline
too would instead read as a beam physically retracting, which is a different statement.

Deliberately NO per-instance phase offset, unlike M_PulseCircle: that one hashes ObjectPositionWS,
which is stable only for something that sits still. A beam is attached to a moving character, so the
hash would re-evaluate as the player walks and warp the pulse mid-cast. Beams are also short-lived
and rarely on screen together, so there is nothing to desynchronise in the first place.

Opacity is left unconnected (defaults to 1): unlike the circle there is no shape to mask out, the
whole quad is the beam.

Also creates MI_PulseBeam: use THAT one. A base material's parameter defaults cannot be tuned from
the Details panel, an instance's can, and a MID for runtime animation parents off it.
"""
import unreal

PKG_PATH = "/Game/VFX/Generic/Materials"
NAME = "M_PulseBeam"
INSTANCE_NAME = "MI_PulseBeam"
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
    # defaulted Step input still compiles, into a frame that is simply the wrong size. Never call
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


def axis_distance(source, red, x, y):
    """abs((component - 0.5) * 2): 0 at the middle of the quad, 1 at both of that axis' edges."""
    mask = expr(unreal.MaterialExpressionComponentMask, x, y)
    mask.set_editor_property("r", red)
    mask.set_editor_property("g", not red)
    mask.set_editor_property("b", False)
    mask.set_editor_property("a", False)
    connect(source, mask, "")

    centered = expr(unreal.MaterialExpressionSubtract, x + 200, y)
    centered.set_editor_property("const_b", 0.5)
    connect(mask, centered, "A")

    distance = expr(unreal.MaterialExpressionAbs, x + 560, y)
    connect(multiply(centered, None, 2.0, x + 380, y), distance, "")
    return mask, distance


# --- quad space ---
tex = expr(unreal.MaterialExpressionTextureCoordinate, -1900, -180)
along, du = axis_distance(tex, True, -1700, -280)
_, dv = axis_distance(tex, False, -1700, -60)

# --- Moira beam's own parameters ---
length = scalar("Beam_Length", 1000.0, -1900, 180)
width = scalar("Beam_Width", 100.0, -1900, 320)
tint = color("Color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0), 620, 200)

# --- outline, uniform on all four sides ---
thickness = scalar("OutlineThickness", 0.08, -1900, 460)

aspect = expr(unreal.MaterialExpressionDivide, -1660, 240)
connect(width, aspect, "A")
connect(length, aspect, "B")

inner_u = expr(unreal.MaterialExpressionOneMinus, -1240, 240)
connect(multiply(thickness, aspect, None, -1440, 240), inner_u, "")

inner_v = expr(unreal.MaterialExpressionOneMinus, -1240, 460)
connect(thickness, inner_v, "")

# Step(Y, X) is 1 where X >= Y, so each of these is 1 only within OutlineThickness of its own edges.
edge_u = expr(unreal.MaterialExpressionStep, -1000, -280)
connect(inner_u, edge_u, "Y")
connect(du, edge_u, "X")

edge_v = expr(unreal.MaterialExpressionStep, -1000, -60)
connect(inner_v, edge_v, "Y")
connect(dv, edge_v, "X")

outline = expr(unreal.MaterialExpressionMax, -780, -180)
connect(edge_u, outline, "A")
connect(edge_v, outline, "B")

fill = expr(unreal.MaterialExpressionOneMinus, -560, -180)
connect(outline, fill, "")

# --- Pulse = sin(u * PulseCount - Time * PulseSpeed) * 0.35 + 0.65 ---
count = scalar("PulseCount", 3.0, -1900, 640)
speed = scalar("PulseSpeed", 1.0, -1900, 780)
time = expr(unreal.MaterialExpressionTime, -1900, 920)

phase = expr(unreal.MaterialExpressionSubtract, -1240, 700)
connect(multiply(along, count, None, -1440, 640), phase, "A")
connect(multiply(time, speed, None, -1440, 840), phase, "B")

wave = expr(unreal.MaterialExpressionSine, -1040, 700)
connect(phase, wave, "")

pulse = expr(unreal.MaterialExpressionAdd, -680, 700)
pulse.set_editor_property("const_b", 0.65)
connect(multiply(wave, None, 0.35, -860, 700), pulse, "A")

# --- Wipe = Step(u, 1 - DurationSpent) ---
spent = scalar("DurationSpent", 0.0, -1900, 1060)

alive = expr(unreal.MaterialExpressionOneMinus, -1660, 1060)
connect(spent, alive, "")

# Step(Y, X) is 1 where X >= Y, so the span from the caster up to the surviving fraction stays lit.
wipe = expr(unreal.MaterialExpressionStep, -1400, 1000)
connect(along, wipe, "Y")
connect(alive, wipe, "X")

# --- Emissive = (OutlineColor * Outline + InsideColor * Fill * Pulse * Wipe) * Color ---
outline_color = color("OutlineColor", unreal.LinearColor(1.0, 1.0, 1.0, 1.0), -400, -420)
inside_color = color("InsideColor", unreal.LinearColor(0.15, 0.6, 1.0, 1.0), -400, 440)

alive_fill = multiply(multiply(fill, pulse, None, -180, 200), wipe, None, 20, 260)

lit = expr(unreal.MaterialExpressionAdd, 500, 0)
connect(multiply(outline_color, outline, None, 260, -280), lit, "A")
connect(multiply(inside_color, alive_fill, None, 260, 320), lit, "B")

# Master tint last, so it scales the finished look rather than one half of it.
emissive = multiply(lit, tint, None, 780, 20)

mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

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

unreal.log(f"PULSEBEAM::built {FULL} + {INSTANCE_NAME}")
