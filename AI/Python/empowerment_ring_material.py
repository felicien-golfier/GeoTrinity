"""Sprite materials for NS_Empowerment: a dashed ground ring, a spark, and a hard-edged shard.

The camera is orthographic and looks straight down, so a camera-facing sprite already lies flat on the
ground plane - a ring drawn in sprite UV space is a ground ring, with no mesh and no orientation module.

Every mask is shaped to hold an edge rather than fade to a blur: a soft round falloff at these sizes reads
as a smudge, and a screen full of smudges is what makes an aura look like noise instead of a shape.

Each material fades itself over the particle's life. The Niagara sprite factory feeds Particle.RelativeTime
from the NormalizedAge binding, so the ramp costs nothing on the stack - and the template's own ScaleColor,
whose alpha reads ~0 at every age, is unusable anyway.

Emissive carries the colour and Opacity carries the mask. Additive resolves to Emissive*Opacity+Dest, so
feeding the mask to both would square its falloff and pinch the shape thin.
"""

import unreal
import traceback

PACKAGE_PATH = "/Game/Art/VFX/Generic/Materials"
RING_ASSET = "M_EmpowerRing"
DOT_ASSET = "M_EmpowerDot"
SHARD_ASSET = "M_EmpowerShard"
WISP_ASSET = "M_EmpowerWisp"

MEL = unreal.MaterialEditingLibrary
LOG = []


def record(label, value):
    LOG.append("%-46s %s" % (label, value))
    return value


def clear_asset(package_path):
    """Frees a package path; one the editor still holds is renamed aside instead."""
    if not unreal.EditorAssetLibrary.does_asset_exist(package_path):
        return True
    return (unreal.EditorAssetLibrary.delete_asset(package_path)
            or unreal.EditorAssetLibrary.rename_asset(package_path, package_path + "_Superseded"))


class Graph:
    """Node factory that fails loudly - an unknown pin name silently leaves an input on its default."""

    def __init__(self, material):
        self.material = material
        self.x = -2400
        self.y = 0

    def node(self, cls, dx=260, dy=0, **properties):
        self.x += dx
        self.y += dy
        expression = MEL.create_material_expression(self.material, cls, self.x, self.y)
        for key, value in properties.items():
            expression.set_editor_property(key, value)
        return expression

    def link(self, source, source_pin, target, target_pin):
        ok = MEL.connect_material_expressions(source, source_pin, target, target_pin)
        record("link %s.%s -> %s.%s" % (source.get_name(), source_pin, target.get_name(), target_pin), ok)
        if not ok:
            raise RuntimeError("unknown pin %s -> %s" % (source_pin, target_pin))
        return ok

    def out(self, source, source_pin, prop):
        return record("out %s -> %s" % (source.get_name(), prop),
                      MEL.connect_material_property(source, source_pin, prop))


def scalar(graph, name, default, dx=0, dy=0):
    return graph.node(unreal.MaterialExpressionScalarParameter, dx=dx, dy=dy,
                      parameter_name=name, default_value=default)


def centred_uv(g):
    """Sprite UV recentred to [-1,1], so radius 1 is the quad's half-extent."""
    uv = g.node(unreal.MaterialExpressionTextureCoordinate)
    centred = g.node(unreal.MaterialExpressionSubtract, const_b=0.5)
    g.link(uv, "", centred, "A")
    uvn = g.node(unreal.MaterialExpressionMultiply, const_b=2.0)
    g.link(centred, "", uvn, "A")
    return uvn


def radius(g, uvn):
    dot = g.node(unreal.MaterialExpressionDotProduct)
    g.link(uvn, "", dot, "A")
    g.link(uvn, "", dot, "B")
    dist = g.node(unreal.MaterialExpressionSquareRoot)
    g.link(dot, "", dist, "")
    return dist


def age_fade(g, rise, fall):
    """A trapezoid over normalized age: in over `rise`, held, out over `fall`."""
    age = g.node(unreal.MaterialExpressionParticleRelativeTime, dy=-620)
    rising = g.node(unreal.MaterialExpressionDivide, const_b=rise)
    g.link(age, "", rising, "A")
    rising_clamped = g.node(unreal.MaterialExpressionSaturate)
    g.link(rising, "", rising_clamped, "")
    remaining = g.node(unreal.MaterialExpressionOneMinus, dx=-520, dy=200)
    g.link(age, "", remaining, "")
    falling = g.node(unreal.MaterialExpressionDivide, const_b=fall)
    g.link(remaining, "", falling, "A")
    falling_clamped = g.node(unreal.MaterialExpressionSaturate)
    g.link(falling, "", falling_clamped, "")
    fade = g.node(unreal.MaterialExpressionMin, dy=-100)
    g.link(rising_clamped, "", fade, "A")
    g.link(falling_clamped, "", fade, "B")
    return fade


def finish(g, material, mask, fade):
    """Colour comes from the particle; the mask and the age ramp ride the alpha.

    ParticleColor's unnamed output is rgb alone, so alpha has to come off its own named pin - masking the
    default output for alpha asks for a fourth component that is not there and fails to compile. Emissive
    takes that unnamed output rather than a named "RGB" pin, which resolves to white and throws the palette
    away without failing the connection.
    """
    shaped = g.node(unreal.MaterialExpressionMultiply, dy=-300)
    g.link(mask, "", shaped, "A")
    g.link(fade, "", shaped, "B")
    particle = g.node(unreal.MaterialExpressionParticleColor, dy=-260)
    opacity = g.node(unreal.MaterialExpressionMultiply, dy=260)
    g.link(shaped, "", opacity, "A")
    g.link(particle, "A", opacity, "B")
    g.out(particle, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    g.out(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    # Blend mode and shading model go on after wiring, never before.
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_niagara_sprites", True)
    MEL.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material.get_path_name())
    record("saved", material.get_path_name())


def create(asset_name):
    clear_asset("%s/%s" % (PACKAGE_PATH, asset_name))
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, PACKAGE_PATH, unreal.Material, unreal.MaterialFactoryNew())
    record("created", material.get_path_name())
    return material, Graph(material)


def build_ring():
    """A thin dashed annulus. The dashes are what make its rotation visible; a plain ring cannot show spin."""
    material, g = create(RING_ASSET)
    uvn = centred_uv(g)
    dist = radius(g, uvn)

    ring_radius = scalar(g, "RingRadius", 0.78, dx=0, dy=-320)
    offset = g.node(unreal.MaterialExpressionSubtract, dy=320)
    g.link(dist, "", offset, "A")
    g.link(ring_radius, "", offset, "B")
    absolute = g.node(unreal.MaterialExpressionAbs)
    g.link(offset, "", absolute, "")
    ring_width = scalar(g, "RingWidth", 0.055, dx=0, dy=-320)
    normalised = g.node(unreal.MaterialExpressionDivide, dy=320)
    g.link(absolute, "", normalised, "A")
    g.link(ring_width, "", normalised, "B")
    inverted = g.node(unreal.MaterialExpressionOneMinus)
    g.link(normalised, "", inverted, "")
    clamped = g.node(unreal.MaterialExpressionSaturate)
    g.link(inverted, "", clamped, "")
    # An exponent below one flattens the band's core and steepens its edge, which is what keeps a thin
    # ring reading as a line rather than a blur.
    sharpness = scalar(g, "EdgeSharpness", 0.55, dx=0, dy=-320)
    band = g.node(unreal.MaterialExpressionPower, dy=320)
    g.link(clamped, "", band, "Base")
    g.link(sharpness, "", band, "Exp")

    u = g.node(unreal.MaterialExpressionComponentMask, dy=520, r=True, g=False, b=False, a=False)
    g.link(uvn, "", u, "")
    v = g.node(unreal.MaterialExpressionComponentMask, dx=0, dy=180, r=False, g=True, b=False, a=False)
    g.link(uvn, "", v, "")
    angle = g.node(unreal.MaterialExpressionArctangent2Fast, dy=-90)
    g.link(v, "", angle, "Y")
    g.link(u, "", angle, "X")
    turns = g.node(unreal.MaterialExpressionMultiply, const_b=0.15915494)  # 1/2pi
    g.link(angle, "", turns, "A")
    segments = scalar(g, "Segments", 6.0, dx=0, dy=-260)
    stepped = g.node(unreal.MaterialExpressionMultiply, dy=260)
    g.link(turns, "", stepped, "A")
    g.link(segments, "", stepped, "B")
    repeated = g.node(unreal.MaterialExpressionFrac)
    g.link(stepped, "", repeated, "")
    doubled = g.node(unreal.MaterialExpressionMultiply, const_b=2.0)
    g.link(repeated, "", doubled, "A")
    biased = g.node(unreal.MaterialExpressionSubtract, const_b=1.0)
    g.link(doubled, "", biased, "A")
    folded = g.node(unreal.MaterialExpressionAbs)
    g.link(biased, "", folded, "")
    wave = g.node(unreal.MaterialExpressionOneMinus)
    g.link(folded, "", wave, "")
    widened = g.node(unreal.MaterialExpressionPower, const_exponent=0.7)
    g.link(wave, "", widened, "Base")
    dash_depth = scalar(g, "DashDepth", 0.65, dx=0, dy=-260)
    dashes = g.node(unreal.MaterialExpressionLinearInterpolate, dy=260, const_a=1.0)
    g.link(widened, "", dashes, "B")
    g.link(dash_depth, "", dashes, "Alpha")

    mask = g.node(unreal.MaterialExpressionMultiply, dy=-520)
    g.link(band, "", mask, "A")
    g.link(dashes, "", mask, "B")
    finish(g, material, mask, age_fade(g, 0.10, 0.25))
    return "\n".join(LOG)


def build_dot():
    """A spark: a tight core with almost no skirt, so a scatter of them reads as points, not haze."""
    material, g = create(DOT_ASSET)
    dist = radius(g, centred_uv(g))
    inverted = g.node(unreal.MaterialExpressionOneMinus)
    g.link(dist, "", inverted, "")
    clamped = g.node(unreal.MaterialExpressionSaturate)
    g.link(inverted, "", clamped, "")
    falloff = scalar(g, "Falloff", 4.0, dx=0, dy=-300)
    mask = g.node(unreal.MaterialExpressionPower, dy=300)
    g.link(clamped, "", mask, "Base")
    g.link(falloff, "", mask, "Exp")
    finish(g, material, mask, age_fade(g, 0.15, 0.45))
    return "\n".join(LOG)


def build_shard():
    """A hard-edged triangle with a thin rim.

    Three half-plane distances share one centre, so their maximum is the distance to the triangle's edge:
    a countable silhouette from directly above, which a round sprite can never be.
    """
    material, g = create(SHARD_ASSET)
    uvn = centred_uv(g)

    planes = []
    for index, (nx, ny) in enumerate(((0.0, 1.0), (0.866, -0.5), (-0.866, -0.5))):
        normal = g.node(unreal.MaterialExpressionConstant2Vector, dx=(0 if index else 260),
                        dy=(220 if index else 0), r=nx, g=ny)
        plane = g.node(unreal.MaterialExpressionDotProduct, dx=0, dy=-110)
        g.link(uvn, "", plane, "A")
        g.link(normal, "", plane, "B")
        planes.append(plane)
    outer = g.node(unreal.MaterialExpressionMax, dy=-220)
    g.link(planes[0], "", outer, "A")
    g.link(planes[1], "", outer, "B")
    edge = g.node(unreal.MaterialExpressionMax)
    g.link(outer, "", edge, "A")
    g.link(planes[2], "", edge, "B")

    size = scalar(g, "Size", 0.55, dx=0, dy=-320)
    inside = g.node(unreal.MaterialExpressionSubtract, dy=320)
    g.link(size, "", inside, "A")
    g.link(edge, "", inside, "B")
    softness = scalar(g, "Softness", 0.09, dx=0, dy=-320)
    scaled = g.node(unreal.MaterialExpressionDivide, dy=320)
    g.link(inside, "", scaled, "A")
    g.link(softness, "", scaled, "B")
    fill = g.node(unreal.MaterialExpressionSaturate)
    g.link(scaled, "", fill, "")

    # A rim outside the fill keeps the shard from reading as a flat sticker at small sizes.
    away = g.node(unreal.MaterialExpressionAbs, dy=420)
    g.link(inside, "", away, "")
    rim_width = scalar(g, "RimWidth", 0.28, dx=0, dy=-300)
    rim_scaled = g.node(unreal.MaterialExpressionDivide, dy=300)
    g.link(away, "", rim_scaled, "A")
    g.link(rim_width, "", rim_scaled, "B")
    rim_inverted = g.node(unreal.MaterialExpressionOneMinus)
    g.link(rim_scaled, "", rim_inverted, "")
    rim_clamped = g.node(unreal.MaterialExpressionSaturate)
    g.link(rim_inverted, "", rim_clamped, "")
    rim_strength = scalar(g, "RimStrength", 0.5, dx=0, dy=-300)
    rim = g.node(unreal.MaterialExpressionMultiply, dy=300)
    g.link(rim_clamped, "", rim, "A")
    g.link(rim_strength, "", rim, "B")

    combined = g.node(unreal.MaterialExpressionAdd, dy=-420)
    g.link(fill, "", combined, "A")
    g.link(rim, "", combined, "B")
    mask = g.node(unreal.MaterialExpressionSaturate)
    g.link(combined, "", mask, "")
    finish(g, material, mask, age_fade(g, 0.08, 0.30))
    return "\n".join(LOG)


def build_wisp():
    """A ribbon strand: bright along its spine, tapering to nothing at both ends.

    Ribbon UVs run along the strand in u and across it in v, so the whole shape - the soft round profile
    and the tapered head and tail the reference wisps have - comes out of the material and needs no curve
    on the stack.
    """
    material, g = create(WISP_ASSET)
    uv = g.node(unreal.MaterialExpressionTextureCoordinate)
    across = g.node(unreal.MaterialExpressionComponentMask, r=False, g=True, b=False, a=False)
    g.link(uv, "", across, "")
    centred = g.node(unreal.MaterialExpressionSubtract, const_b=0.5)
    g.link(across, "", centred, "A")
    doubled = g.node(unreal.MaterialExpressionMultiply, const_b=2.0)
    g.link(centred, "", doubled, "A")
    folded = g.node(unreal.MaterialExpressionAbs)
    g.link(doubled, "", folded, "")
    profile = g.node(unreal.MaterialExpressionOneMinus)
    g.link(folded, "", profile, "")
    profile_clamped = g.node(unreal.MaterialExpressionSaturate)
    g.link(profile, "", profile_clamped, "")
    edge = scalar(g, "EdgeSharpness", 0.5, dx=0, dy=-300)
    core = g.node(unreal.MaterialExpressionPower, dy=300)
    g.link(profile_clamped, "", core, "Base")
    g.link(edge, "", core, "Exp")

    # 4u(1-u) peaks mid-strand and reaches zero at both ends, which is the taper without a texture.
    along = g.node(unreal.MaterialExpressionComponentMask, dy=420, r=True, g=False, b=False, a=False)
    g.link(uv, "", along, "")
    remaining = g.node(unreal.MaterialExpressionOneMinus)
    g.link(along, "", remaining, "")
    product = g.node(unreal.MaterialExpressionMultiply)
    g.link(along, "", product, "A")
    g.link(remaining, "", product, "B")
    taper = g.node(unreal.MaterialExpressionMultiply, const_b=4.0)
    g.link(product, "", taper, "A")
    taper_clamped = g.node(unreal.MaterialExpressionSaturate)
    g.link(taper, "", taper_clamped, "")

    mask = g.node(unreal.MaterialExpressionMultiply, dy=-420)
    g.link(core, "", mask, "A")
    g.link(taper_clamped, "", mask, "B")
    finish(g, material, mask, age_fade(g, 0.12, 0.40))
    material.set_editor_property("used_with_niagara_ribbons", True)
    MEL.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material.get_path_name())
    return "\n".join(LOG)


def build_all():
    build_ring()
    build_dot()
    build_shard()
    return build_wisp()


if __name__ == "__main__":
    report = "%sGeoTrinity_EmpowerRingMaterial.txt" % unreal.Paths.project_saved_dir()
    try:
        open(report, "w").write(build_all())
    except Exception:
        open(report, "w").write("\n".join(LOG) + "\n\nFAILED\n" + traceback.format_exc())
