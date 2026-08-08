"""Create M_DeployableOutline — the cel-shading outline post-process material.

Deployables render into the custom-depth pass with their AGeoDeployableBase::OutlineColor slot index
(EGeoColor ordinal + 1; 0 = no outline) as their stencil value. This material draws a constant-width
line wherever the stencil CHANGES, so the shapes themselves are untouched, and tints that line with
the palette color the stencil index names.

Per pixel:
  center    = CustomStencil here
  neighbour = max( CustomStencil at the 4 taps one Thickness away in X and Y )
  slot      = max(neighbour, center)
  Outline   = saturate(abs(neighbour - center))   -> 1 on any stencil discontinuity
  Color     = Palette texel (slot - 0.5) / PaletteSize
  Result    = lerp(PostProcessInput0, Color, Outline)

The gate is a discontinuity and not "outside a shape" so that overlapping deployables keep the border
between them. Testing center == 0 instead suppresses the line on every tagged pixel, which erases the
inner shape's edge and merges the pair into one silhouette — and once a zone material writes custom
depth, it swallows the outline of anything standing inside it. A pixel strictly inside one shape reads
the same index on all four taps and stays dark either way, so single deployables look identical. Two
overlapping deployables that share a slot still merge: 8 stencil bits carry the palette index, with no
room for a per-object id to separate them.

Colors arrive as a texture, not as named parameters: AGeoGameCamera::ApplyOutlineMaterial builds a
1 x SlotCount lookup from UGameDataSettings::ColorPalette (one texel per EGeoColor ordinal) and sets
it on a MID of MI_DeployableOutline, with the texel count in PaletteSize. That is the only way a
material can index a palette — collection parameters are name-keyed and cannot be indexed — and it
is what keeps this material enum-agnostic: adding an EGeoColor slot needs NO rerun of this script,
the texture simply gets one more texel. Sampling is point/clamped, so no two slots ever blend.

Outside PIE the material shows its default texture, since nothing fills the palette until BeginPlay.

Thickness is in pixels: the tap offset is the scene texture InvSize * Thickness, so the line
keeps its width at every resolution. The stencil taps are point-sampled (SceneTexture
bFiltered stays false) — filtering would blend stencil indices into meaningless values.

Blendable location must stay in the "Scene Color After DOF" family (the UI's old "before
tonemapping"): a stencil is either set or not, so Outline is binary and the line has nothing
smoothing it of its own. That location is the last one running at *rendering* resolution — the
size CustomStencil is stored at — and it sits upstream of TSR/TAA/FXAA/SMAA, so the engine's
anti-aliasing resolves the line like any geometry edge. Every later location (Before Bloom,
Replacing Tonemapper, After Tonemapping) runs at display resolution with TSR/TAAU and after
the AA passes: there the line keeps hard stair-steps, and a screen percentage below 100 blows
up each stencil texel into a block. Pairs with r.CustomDepthTemporalAAJitter left at the engine
default in DefaultEngine.ini, so the custom-depth pass carries the main pass' jitter and TSR
accumulates the line's sub-pixel positions instead of smearing it.

Silhouettes come from the depth pass, not from geometry, so shader-defined shapes outline
correctly too (M_HealingZone is Masked: its circle, not its quad, gets the line). Translucent
and additive materials do not write custom depth unless their material ticks
Translucency > Allow Custom Depth Writes.

Requires r.CustomDepth = 3 in Config/DefaultEngine.ini (stencil read/write) — already set.
Also creates MI_DeployableOutline: assign THAT one to BP_GeoCam > Camera|Outline > Outline Material
(the camera installs it as a blendable itself), and tune Thickness on it.
"""
import unreal

PKG_PATH = "/Game/Art/Mat"
NAME = "M_DeployableOutline"
INSTANCE_NAME = "MI_DeployableOutline"
FULL = f"{PKG_PATH}/{NAME}.{NAME}"

# Only ever shown before BeginPlay swaps the real palette in, but it has to be non-sRGB or the
# LinearColor sampler refuses to compile — engine content offers few of those.
DEFAULT_PALETTE_TEXTURE = "/Engine/EngineMaterials/DefaultBloomKernel"

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

# Rebuilt in place, never deleted and recreated: the camera references the instance and the
# instance references the material, so a delete stops on the "still referenced / force delete"
# modal, which blocks the game thread with nobody to answer it.
#
# Existence is decided by attempting the load, not by does_asset_exist: that one answers from the asset
# registry, which reports "missing" for everything it has not scanned yet. Run this on a just-opened
# editor and it takes the create branch against an asset that is already there.
mat = unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{NAME}")
if mat:
    # UMaterialEditingLibrary::DeleteAllMaterialExpressions range-iterates the very array each delete
    # removes from, so one call drops only every other node. Repeat until the graph is really empty —
    # leftovers keep the material referencing whatever they pointed at, and pile up on every rerun.
    while mel.get_num_material_expressions(mat):
        before = mel.get_num_material_expressions(mat)
        mel.delete_all_material_expressions(mat)
        assert mel.get_num_material_expressions(mat) < before, "material graph would not clear"
else:
    mat = at.create_asset(NAME, PKG_PATH, unreal.Material, unreal.MaterialFactoryNew())

assert mat, f"{NAME}: neither loaded nor created"

# Domain before the first node, not with the settings at the end: a SceneTexture expression is only
# valid on a post-process material, and create_material_expression just returns None on any other
# domain — no error, no log. A rebuild inherits the domain from the saved asset, which is the only
# reason setting it last ever worked; a from-scratch create could never have got past the first tap.
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_DOF)


def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def connect(source, source_output, target, target_input):
    # A wrong pin name is not an error, it is a no-op that leaves the input on its default — and a
    # texture sample defaulting its UVs samples screen space, which looks like a working outline
    # whose color drifts across the viewport. Never call the library's connect directly.
    assert mel.connect_material_expressions(source, source_output, target, target_input), (
        f"{type(source).__name__}.{source_output or 'out'} -> {type(target).__name__}.{target_input}")


def stencil(x, y):
    node = expr(unreal.MaterialExpressionSceneTexture, x, y)
    node.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_CUSTOM_STENCIL)
    return node


def mask(source, source_output, x, y, r, g, b=False):
    node = expr(unreal.MaterialExpressionComponentMask, x, y)
    node.set_editor_property("r", r)
    node.set_editor_property("g", g)
    node.set_editor_property("b", b)
    node.set_editor_property("a", False)
    connect(source, source_output, node, "")
    return node


def maximum(a, a_output, b, b_output, x, y):
    node = expr(unreal.MaterialExpressionMax, x, y)
    connect(a, a_output, node, "A")
    connect(b, b_output, node, "B")
    return node


# --- viewport UV, and the center tap (its InvSize output also sizes the offsets) ---
uv = expr(unreal.MaterialExpressionTextureCoordinate, -1900, -520)
center_tex = stencil(-1900, -260)

thickness = expr(unreal.MaterialExpressionScalarParameter, -1900, 60)
thickness.set_editor_property("parameter_name", "Thickness")
thickness.set_editor_property("default_value", 2.0)

# --- offset of one tap, in UV space: InvSize (1/buffer size) * Thickness pixels ---
offset = expr(unreal.MaterialExpressionMultiply, -1650, -60)
connect(center_tex, "InvSize", offset, "A")
connect(thickness, "", offset, "B")

offset_x = mask(offset, "", -1450, -140, True, False)
offset_y = mask(offset, "", -1450, 20, False, True)

zero = expr(unreal.MaterialExpressionConstant, -1450, 180)
zero.set_editor_property("r", 0.0)

uv_offset_x = expr(unreal.MaterialExpressionAppendVector, -1250, -140)
connect(offset_x, "", uv_offset_x, "A")
connect(zero, "", uv_offset_x, "B")

uv_offset_y = expr(unreal.MaterialExpressionAppendVector, -1250, 40)
connect(zero, "", uv_offset_y, "A")
connect(offset_y, "", uv_offset_y, "B")


# --- the 4 neighbour stencil taps ---
def tap(uv_offset, offset_class, y):
    uv_tap = expr(offset_class, -1000, y)
    connect(uv, "", uv_tap, "A")
    connect(uv_offset, "", uv_tap, "B")
    node = stencil(-780, y)
    connect(uv_tap, "", node, "")
    return node


right = tap(uv_offset_x, unreal.MaterialExpressionAdd, -900)
left = tap(uv_offset_x, unreal.MaterialExpressionSubtract, -700)
up = tap(uv_offset_y, unreal.MaterialExpressionAdd, -500)
down = tap(uv_offset_y, unreal.MaterialExpressionSubtract, -300)

# The neighbour max keeps the stencil INDEX (not just 0/1) — the palette lookup below reads it.
# Two touching or overlapping deployables resolve to the higher index along their shared border, which
# is why EGeoColor declares its Deployable* slots in ascending priority — see GeoColor.h.
neighbour = maximum(maximum(right, "Color", left, "Color", -520, -820),
                    "",
                    maximum(up, "Color", down, "Color", -520, -420),
                    "",
                    -320, -620)

neighbour_value = mask(neighbour, "", -140, -620, True, False)
center_value = mask(center_tex, "Color", -140, -260, True, False)

# --- Outline = saturate(abs(neighbour - center)) ---
# Stencil values are integers, so neighbours that differ at all differ by 1 and saturate to a
# full-strength line; no scaling is needed to turn the difference into a mask.
stencil_step = expr(unreal.MaterialExpressionSubtract, 40, -440)
connect(neighbour_value, "", stencil_step, "A")
connect(center_value, "", stencil_step, "B")

stencil_step_size = expr(unreal.MaterialExpressionAbs, 220, -440)
connect(stencil_step, "", stencil_step_size, "")

outline = expr(unreal.MaterialExpressionSaturate, 400, -440)
connect(stencil_step_size, "", outline, "")

# --- Color = the palette texel the stencil index points at ---
# The higher of the two sides owns the border, which is why EGeoColor declares its Deployable* slots in
# ascending priority — see GeoColor.h. Reading `neighbour` alone would work only for the outside-only
# gate: on the inner side of a shared border the neighbour is the lower slot.
outline_slot = maximum(neighbour_value, "", center_value, "", -140, -60)

# Texel i sits at (i + 0.5) / PaletteSize and holds EGeoColor ordinal i, and the stencil is that
# ordinal + 1 — so the sample lands at (slot - 0.5) / PaletteSize. Where slot is 0 the UV goes negative
# and clamps to texel 0, which never shows: Outline is 0 on those pixels.
texel_offset = expr(unreal.MaterialExpressionSubtract, 60, 200)
texel_offset.set_editor_property("const_b", 0.5)
connect(outline_slot, "", texel_offset, "A")

palette_size = expr(unreal.MaterialExpressionScalarParameter, 60, 380)
palette_size.set_editor_property("parameter_name", "PaletteSize")
# Only ever read before BeginPlay overwrites it with GeoColor::SlotCount, so it just needs to match that
# count to keep the editor viewport honest.
palette_size.set_editor_property("default_value", 15.0)

palette_u = expr(unreal.MaterialExpressionDivide, 300, 260)
connect(texel_offset, "", palette_u, "A")
connect(palette_size, "", palette_u, "B")

row_v = expr(unreal.MaterialExpressionConstant, 300, 440)
row_v.set_editor_property("r", 0.5)

palette_uv = expr(unreal.MaterialExpressionAppendVector, 500, 300)
connect(palette_u, "", palette_uv, "A")
connect(row_v, "", palette_uv, "B")

palette = expr(unreal.MaterialExpressionTextureSampleParameter2D, 700, 240)
palette.set_editor_property("parameter_name", "Palette")
palette.set_editor_property("texture", unreal.EditorAssetLibrary.load_asset(DEFAULT_PALETTE_TEXTURE))
palette.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
# "UVs", not "Coordinates": inputs are matched by their SHORTENED display name.
connect(palette_uv, "", palette, "UVs")

# --- Result = lerp(scene, Color, Outline) ---
scene = expr(unreal.MaterialExpressionSceneTexture, 700, -300)
scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)

# SceneTexture Color is a float4 — the lerp needs both sides at the width the emissive output takes,
# and the palette's authored alpha has no meaning for an outline.
scene_color = mask(scene, "Color", 880, -300, True, True, True)

result = expr(unreal.MaterialExpressionLinearInterpolate, 1060, -160)
connect(scene_color, "", result, "A")
connect(palette, "RGB", result, "B")
connect(outline, "", result, "Alpha")

mel.connect_material_property(result, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(f"{PKG_PATH}/{NAME}")

# The instance is what goes on the camera: parameter defaults on a base material can't be
# tuned from the Details panel, an instance's can. Left alone on a rerun so tuned values live.
if not unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{INSTANCE_NAME}"):
    instance = at.create_asset(INSTANCE_NAME, PKG_PATH, unreal.MaterialInstanceConstant,
                               unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(instance, mat)
    mel.set_material_instance_scalar_parameter_value(instance, "Thickness", 2.0)
    unreal.EditorAssetLibrary.save_asset(f"{PKG_PATH}/{INSTANCE_NAME}")

unreal.log(f"OUTLINEMAT::built {FULL} + {INSTANCE_NAME}")
