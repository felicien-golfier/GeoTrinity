"""Creates M_GeoShape — the one particle material every geometric effect is drawn with.

A regular polygon, solid or hollow, crisp at any size, with no texture. Per pixel, in sprite UV space:

  p     = (TexCoord - 0.5) * 2            -> quad space, [-1,1] on both axes
  r     = length(p)                       -> 0 at centre, 1 at the edge midpoints
  th    = atan2(p.y, p.x) / 2pi           -> angle in TURNS, which is the unit Cosine already works in
  k     = floor(th * N + 0.5) / N         -> the nearest sector axis, also in turns
  d     = r * cos(2pi * (k - th))         -> distance to the polygon's edge, measured along that axis

d is the polygon's signed distance in the only form that matters here: it equals the apothem exactly on
an edge, whatever direction the pixel lies in. Everything else is thresholds on it.

  A     = Fit * cos(pi / N)               -> the apothem that puts every polygon's VERTICES on one circle
  solid = 1 - smoothstep(A-f, A+f, d)
  inner = 1 - smoothstep(A-T-f, A-T+f, d) -> the same polygon shrunk by the outline thickness
  shape = lerp(solid - inner, solid, Fill)
  dash  = step(frac(th * Dashes), DashDuty)
  glow  = saturate(1 - d)^3 * Glow
  Emissive = ParticleColor.rgb * (shape * dash + glow)
  Opacity  = ParticleColor.a

The apothem has to follow N or the shapes stop being comparable: for a fixed d threshold a triangle's
corners reach twice as far as its edges, so a triangle and a hexagon authored at one particle size would
render at wildly different apparent sizes and a triangle would be clipped by its own quad. Deriving A from
cos(pi/N) inscribes every polygon in the same circle instead, which is what makes one sprite size mean one
size across the whole shape vocabulary. N below 3 is not a polygon, so it is clamped; N at 64 is a circle,
which is how rings are drawn without a second material.

Fill is a lerp rather than a switch so a shape can be caught halfway, and it is the sum of a material scalar
and dynamic parameter 1: an emitter that never writes dynamic parameters still gets the instance's own fill,
and one that writes them can alternate hollow and solid across particles from a single emitter.

Dashes cut the shape into equal angular pieces — a rotating ring reads as nothing without them, since a
circle turning is a circle. Zero dashes leaves frac(0) = 0, which every duty cycle passes, so the default is
continuous with no branch.

Glow is the halo, and it is deliberately shaped by d rather than by r: the falloff follows the polygon, so a
hexagon glows hexagonally. It is added, never blended, so it can only ever brighten the shape it hugs.

Feather is in the same units as d, meaning fractions of the sprite's half-width, so a big shell wants a much
smaller value than a small shard — an outline is only crisp when its feather is about a pixel.

Additive, unlit, two-sided, flagged for sprites, meshes and ribbons so one material serves every renderer.
Instances carry the shape vocabulary; nothing in the game should reference the base material directly.

Run via mcp-unreal execute_script. Reference: AI/ArtDirection.md, AI/MCP/MCP_Material.md.
"""
import unreal

FOLDER = "/Game/Art/VFX/Generic/Materials"
INSTANCE_FOLDER = "/Game/Art/VFX/Generic/Materials/MatInstances"
NAME = "M_GeoShape"

FIT = 0.92  # circumradius as a fraction of the quad's half width; the rest is room for the glow
GLOW_POWER = 3.0  # a soft halo that still falls off fast enough to stay behind the shape

# name, sides, thickness, fill, dashes, duty, glow, feather
# Thickness is a fraction of the shape, so a big frame and a small shard cannot share one instance: the
# frame wants a thin, precise line and the shard wants a stroke it can still be seen through. Feather goes
# the same way — it is only crisp while it covers about a pixel.
INSTANCES = (
    ("MI_GeoShape_Tri", 3.0, 0.22, 0.0, 0.0, 0.5, 0.12, 0.035),
    ("MI_GeoShape_TriThin", 3.0, 0.075, 0.0, 0.0, 0.5, 0.10, 0.012),
    ("MI_GeoShape_TriSolid", 3.0, 0.22, 1.0, 0.0, 0.5, 0.12, 0.035),
    ("MI_GeoShape_Quad", 4.0, 0.20, 0.0, 0.0, 0.5, 0.12, 0.030),
    ("MI_GeoShape_QuadSolid", 4.0, 0.20, 1.0, 0.0, 0.5, 0.12, 0.030),
    ("MI_GeoShape_Hex", 6.0, 0.085, 0.0, 0.0, 0.5, 0.10, 0.014),
    ("MI_GeoShape_HexDash", 6.0, 0.11, 0.0, 6.0, 0.62, 0.10, 0.014),
    ("MI_GeoShape_Ring", 64.0, 0.09, 0.0, 0.0, 0.5, 0.10, 0.010),
    ("MI_GeoShape_RingDash", 64.0, 0.075, 0.0, 8.0, 0.55, 0.10, 0.010),
    ("MI_GeoShape_RingDash4", 64.0, 0.10, 0.0, 4.0, 0.40, 0.10, 0.010),
)

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

# Rebuilt in place: a delete on a material already referenced by an emitter raises a modal nobody can answer.
if unreal.EditorAssetLibrary.does_asset_exist("%s/%s" % (FOLDER, NAME)):
    mat = unreal.EditorAssetLibrary.load_asset("%s/%s" % (FOLDER, NAME))
    while mel.get_num_material_expressions(mat):
        before = mel.get_num_material_expressions(mat)
        mel.delete_all_material_expressions(mat)
        assert mel.get_num_material_expressions(mat) < before, "material graph would not clear"
else:
    mat = at.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def connect(source, target, target_input, source_output=""):
    # A wrong pin name is a no-op, not an error, and an input left on its default still compiles — into a
    # shape that is merely the wrong size. Never call the library's connect directly.
    assert mel.connect_material_expressions(source, source_output, target, target_input), (
        "%s.%s -> %s.%s" % (type(source).__name__, source_output, type(target).__name__, target_input))


def scalar(name, value, x, y):
    node = expr(unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def binary(cls, a, b, x, y, const_a=None, const_b=None):
    node = expr(cls, x, y)
    if a is not None:
        connect(a, node, "A")
    elif const_a is not None:
        node.set_editor_property("const_a", const_a)
    if b is not None:
        connect(b, node, "B")
    elif const_b is not None:
        node.set_editor_property("const_b", const_b)
    return node


def add(a, b, x, y, const_b=None):
    return binary(unreal.MaterialExpressionAdd, a, b, x, y, const_b=const_b)


def sub(a, b, x, y, const_a=None, const_b=None):
    return binary(unreal.MaterialExpressionSubtract, a, b, x, y, const_a=const_a, const_b=const_b)


def mul(a, b, x, y, const_b=None):
    return binary(unreal.MaterialExpressionMultiply, a, b, x, y, const_b=const_b)


def div(a, b, x, y, const_a=None, const_b=None):
    return binary(unreal.MaterialExpressionDivide, a, b, x, y, const_a=const_a, const_b=const_b)


def unary(cls, source, x, y, source_output=""):
    node = expr(cls, x, y)
    connect(source, node, "", source_output)
    return node


def cosine(source, x, y):
    node = expr(unreal.MaterialExpressionCosine, x, y)
    node.set_editor_property("period", 1.0)  # so the input is read in turns, not radians
    connect(source, node, "")
    return node


def band(edge, feather, value, x, y):
    """1 inside `edge`, 0 outside it, over a feather wide enough to cover about a pixel."""
    node = expr(unreal.MaterialExpressionSmoothStep, x, y)
    connect(sub(edge, feather, x - 200, y - 60), node, "Min")
    connect(add(edge, feather, x - 200, y + 60), node, "Max")
    connect(value, node, "Value")
    return unary(unreal.MaterialExpressionOneMinus, node, x + 160, y)


# --- d: distance to the polygon's edge, measured along the nearest sector axis ---
tex = expr(unreal.MaterialExpressionTextureCoordinate, -2600, 0)
p = mul(sub(tex, None, -2400, 0, const_b=0.5), None, -2250, 0, const_b=2.0)

r = unary(unreal.MaterialExpressionSquareRoot,
          binary(unreal.MaterialExpressionDotProduct, p, p, -2100, -200), -1950, -200)

def channel(source, x, y, *keep):
    node = expr(unreal.MaterialExpressionComponentMask, x, y)
    for name in ("r", "g", "b", "a"):
        node.set_editor_property(name, name in keep)
    connect(source, node, "")
    return node


px = channel(p, -2100, 60, "r")
py = channel(p, -2100, 180, "g")

angle = expr(unreal.MaterialExpressionArctangent2, -1950, 120)
connect(py, angle, "Y")
connect(px, angle, "X")
th = div(angle, None, -1800, 120, const_b=6.283185307)

sides = scalar("Sides", 3.0, -2100, 380)
n = binary(unreal.MaterialExpressionMax, sides, None, -1950, 380, const_b=3.0)

k = div(unary(unreal.MaterialExpressionFloor,
              add(mul(th, n, -1650, 200), None, -1500, 200, const_b=0.5), -1350, 200),
        n, -1200, 200)
d = mul(r, cosine(sub(k, th, -1050, 200), -900, 200), -750, 60)

# --- A: the apothem that inscribes every polygon in one circle ---
apothem = mul(cosine(div(None, n, -1650, 480, const_a=0.5), -1500, 480), None, -1350, 480, const_b=FIT)
thickness = mul(apothem, scalar("Thickness", 0.18, -1350, 620), -1200, 560)
inner_edge = sub(apothem, thickness, -1050, 500)

# --- shape: the outline, the solid, and anything between ---
feather = scalar("Feather", 0.03, -1050, 900)
solid = band(apothem, feather, d, -500, 400)
inner = band(inner_edge, feather, d, -500, 700)
outline = sub(solid, inner, -180, 560)

dynamic = expr(unreal.MaterialExpressionDynamicParameter, -700, 1050)
dynamic.set_editor_property("parameter_index", 0)
dynamic.set_editor_property("param_names", ["FillAdd", "Unused1", "Unused2", "Unused3"])
dynamic.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 0.0))

fill_sum = expr(unreal.MaterialExpressionAdd, -450, 960)
connect(scalar("Fill", 0.0, -700, 940), fill_sum, "A")
connect(dynamic, fill_sum, "B")  # output 0 is the first param name, whatever it was renamed to
fill = expr(unreal.MaterialExpressionClamp, -300, 1000)
connect(fill_sum, fill, "")

shape = expr(unreal.MaterialExpressionLinearInterpolate, 40, 600)
connect(outline, shape, "A")
connect(solid, shape, "B")
connect(fill, shape, "Alpha")

# --- dashes: equal angular cuts, so a turning ring has something to read ---
dashes = scalar("Dashes", 0.0, -700, 1300)
duty = scalar("DashDuty", 0.5, -700, 1420)
dash = expr(unreal.MaterialExpressionStep, -180, 1340)
connect(unary(unreal.MaterialExpressionFrac, mul(th, dashes, -500, 1300), -340, 1300), dash, "Y")
connect(duty, dash, "X")

# --- glow: a halo hugging the shape's own edge, held clear of the quad's ---
# Centred on the edge rather than on the middle, because a hollow shape has nothing in its middle: a halo
# that peaked at d = 0 would fill a ring's disc with a soft blob, which is the one thing a ring is not.
# Falling off both ways from the edge serves the solid case too, where the fill already lights the inside.
band_width = scalar("GlowWidth", 0.35, -700, 1780)
edge = unary(unreal.MaterialExpressionSaturate,
             sub(None, div(unary(unreal.MaterialExpressionAbs, sub(d, apothem, -640, 1600), -560, 1600),
                           band_width, -480, 1600),
                 -400, 1600, const_a=1.0), -320, 1600)
faded = expr(unreal.MaterialExpressionPower, -240, 1600)
connect(edge, faded, "Base")
faded.set_editor_property("const_exponent", GLOW_POWER)
# The quad is square and the falloff is not, so without this the sprite's own corners show through the halo.
glow = mul(faded, unary(unreal.MaterialExpressionSaturate,
                        sub(None, r, -480, 1840, const_a=1.0), -320, 1840), -180, 1660)

mask = add(mul(shape, dash, 240, 800), mul(glow, scalar("Glow", 0.35, -180, 1760), 240, 1620), 400, 1000)

particle = expr(unreal.MaterialExpressionParticleColor, 240, 1200)
rgb = expr(unreal.MaterialExpressionComponentMask, 400, 1200)
for channel in ("r", "g", "b"):
    rgb.set_editor_property(channel, True)
rgb.set_editor_property("a", False)
connect(particle, rgb, "")

emissive = mul(rgb, mask, 600, 1100)
assert mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR), "emissive"
assert mel.connect_material_property(particle, "A", unreal.MaterialProperty.MP_OPACITY), "opacity"

mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)
mat.set_editor_property("used_with_niagara_sprites", True)
mat.set_editor_property("used_with_niagara_mesh_particles", True)
mat.set_editor_property("used_with_niagara_ribbons", True)
mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)

for name, n_sides, thick, fill_value, dash_count, dash_duty, glow_value, feather_value in INSTANCES:
    path = "%s/%s" % (INSTANCE_FOLDER, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        instance = unreal.EditorAssetLibrary.load_asset(path)
    else:
        instance = at.create_asset(name, INSTANCE_FOLDER, unreal.MaterialInstanceConstant,
                                   unreal.MaterialInstanceConstantFactoryNew())
    instance.set_editor_property("parent", mat)
    for parameter, value in (("Sides", n_sides), ("Thickness", thick), ("Fill", fill_value),
                             ("Dashes", dash_count), ("DashDuty", dash_duty), ("Glow", glow_value),
                             ("Feather", feather_value)):
        mel.set_material_instance_scalar_parameter_value(instance, parameter, value)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
