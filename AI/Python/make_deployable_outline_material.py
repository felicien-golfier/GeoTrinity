"""Create M_DeployableOutline — the cel-shading outline post-process material — and MPC_GeoColorPalette.

Deployables render into the custom-depth pass with their AGeoDeployableBase::OutlineColor slot index
(EGeoColor ordinal + 1; 0 = no outline) as their stencil value. This material draws a constant-width
line on the pixels just OUTSIDE those silhouettes, so the shapes themselves are untouched, and tints
that line with the palette color the stencil index names.

Per pixel:
  center    = CustomStencil here
  neighbour = max( CustomStencil at the 4 taps one Thickness away in X and Y )
  Outline   = saturate(neighbour) * (1 - saturate(center))   -> 1 only just outside a tagged shape
  Color     = SLOTS[neighbour - 1], selected by the Step/Lerp cascade below
  Result    = lerp(PostProcessInput0, Color, Outline)

The index -> color step is a cascade, not a branch: starting from SLOTS[0], each following slot i is
lerped in with `Step(i + 1, neighbour)` (1 once neighbour >= i + 1), so the highest threshold the
stencil clears wins and the last write is the right color. Exact float compares are safe — the
stencil is an integer stored in a float. MaterialExpressionIf is avoided on purpose: its inline
scalar constants are not reachable from Python (see AI/MCP/MCP_Material.md).

Colors come from MPC_GeoColorPalette, one vector parameter per EGeoColor slot, named after the enum
value. AGeoWorldSettings::BeginPlay fills it from UGameDataSettings::ColorPalette on every level, so
the palette in Project Settings > Game Data Settings > Colors stays the single source of truth. The
collection's own defaults are left at black, which is what the outline renders as outside PIE.

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
Also creates MI_DeployableOutline: put THAT one in BP_GeoCam > CameraComponent > Post Process
> Post Process Materials, and tune Thickness on it.
"""
import unreal

PKG_PATH = "/Game/Art/Mat"
NAME = "M_DeployableOutline"
INSTANCE_NAME = "MI_DeployableOutline"
COLLECTION_NAME = "MPC_GeoColorPalette"
FULL = f"{PKG_PATH}/{NAME}.{NAME}"

# EGeoColor in declaration order, minus Override (no palette color to look up). Position IS the
# meaning: stencil value = index + 1, matching AGeoDeployableBase::ApplyOutlineStencil. Reordering
# this list without reordering the enum silently recolors every deployable.
SLOTS = [
    "Damage",
    "AllyDamage",
    "LethalDamage",
    "Heal",
    "Shield",
    "BothHealAndDamage",
    "DamageReduction",
    "DamageBoost",
    "HealBoost",
    "MoveSpeed",
    "Neutral",
]

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

# --- the palette collection, extended in place: rewriting the whole array would mint new parameter
# --- GUIDs and break every material already pointing at a slot.
if unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{COLLECTION_NAME}"):
    collection = unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{COLLECTION_NAME}")
else:
    collection = at.create_asset(COLLECTION_NAME, PKG_PATH, unreal.MaterialParameterCollection,
                                 unreal.MaterialParameterCollectionFactoryNew())

collection_params = list(collection.get_editor_property("vector_parameters"))
existing_names = [str(param.get_editor_property("parameter_name")) for param in collection_params]
for slot in SLOTS:
    if slot not in existing_names:
        param = unreal.CollectionVectorParameter()
        param.set_editor_property("parameter_name", slot)
        collection_params.append(param)
collection.set_editor_property("vector_parameters", collection_params)
unreal.EditorAssetLibrary.save_asset(f"{PKG_PATH}/{COLLECTION_NAME}")

# Rebuilt in place, never deleted and recreated: the camera references the instance and the
# instance references the material, so a delete stops on the "still referenced / force delete"
# modal, which blocks the game thread with nobody to answer it.
if unreal.EditorAssetLibrary.does_asset_exist(f"{PKG_PATH}/{NAME}"):
    mat = unreal.EditorAssetLibrary.load_asset(f"{PKG_PATH}/{NAME}")
    mel.delete_all_material_expressions(mat)
else:
    mat = at.create_asset(NAME, PKG_PATH, unreal.Material, unreal.MaterialFactoryNew())


def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


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
    mel.connect_material_expressions(source, source_output, node, "")
    return node


def maximum(a, a_output, b, b_output, x, y):
    node = expr(unreal.MaterialExpressionMax, x, y)
    mel.connect_material_expressions(a, a_output, node, "A")
    mel.connect_material_expressions(b, b_output, node, "B")
    return node


def palette_slot(slot, x, y):
    node = expr(unreal.MaterialExpressionCollectionParameter, x, y)
    node.set_editor_property("collection", collection)
    node.set_editor_property("parameter_name", slot)
    # Resolves ParameterId from ParameterName against Collection — without it the node compiles to 0.
    node.post_edit_change()
    return node


# --- viewport UV, and the center tap (its InvSize output also sizes the offsets) ---
uv = expr(unreal.MaterialExpressionTextureCoordinate, -1900, -520)
center_tex = stencil(-1900, -260)

thickness = expr(unreal.MaterialExpressionScalarParameter, -1900, 60)
thickness.set_editor_property("parameter_name", "Thickness")
thickness.set_editor_property("default_value", 2.0)

# --- offset of one tap, in UV space: InvSize (1/buffer size) * Thickness pixels ---
offset = expr(unreal.MaterialExpressionMultiply, -1650, -60)
mel.connect_material_expressions(center_tex, "InvSize", offset, "A")
mel.connect_material_expressions(thickness, "", offset, "B")

offset_x = mask(offset, "", -1450, -140, True, False)
offset_y = mask(offset, "", -1450, 20, False, True)

zero = expr(unreal.MaterialExpressionConstant, -1450, 180)
zero.set_editor_property("r", 0.0)

uv_offset_x = expr(unreal.MaterialExpressionAppendVector, -1250, -140)
mel.connect_material_expressions(offset_x, "", uv_offset_x, "A")
mel.connect_material_expressions(zero, "", uv_offset_x, "B")

uv_offset_y = expr(unreal.MaterialExpressionAppendVector, -1250, 40)
mel.connect_material_expressions(zero, "", uv_offset_y, "A")
mel.connect_material_expressions(offset_y, "", uv_offset_y, "B")


# --- the 4 neighbour stencil taps ---
def tap(uv_offset, offset_class, y):
    uv_tap = expr(offset_class, -1000, y)
    mel.connect_material_expressions(uv, "", uv_tap, "A")
    mel.connect_material_expressions(uv_offset, "", uv_tap, "B")
    node = stencil(-780, y)
    mel.connect_material_expressions(uv_tap, "", node, "")
    return node


right = tap(uv_offset_x, unreal.MaterialExpressionAdd, -900)
left = tap(uv_offset_x, unreal.MaterialExpressionSubtract, -700)
up = tap(uv_offset_y, unreal.MaterialExpressionAdd, -500)
down = tap(uv_offset_y, unreal.MaterialExpressionSubtract, -300)

# The neighbour max keeps the stencil INDEX (not just 0/1) — the palette cascade below branches on it.
# Two touching deployables of different slots resolve to the higher index along their shared border.
neighbour = maximum(maximum(right, "Color", left, "Color", -520, -820),
                    "",
                    maximum(up, "Color", down, "Color", -520, -420),
                    "",
                    -320, -620)

neighbour_value = mask(neighbour, "", -140, -620, True, False)
center_value = mask(center_tex, "Color", -140, -260, True, False)

# --- Outline = saturate(neighbour) * (1 - saturate(center)) ---
neighbour_hit = expr(unreal.MaterialExpressionSaturate, 40, -620)
mel.connect_material_expressions(neighbour_value, "", neighbour_hit, "")

center_hit = expr(unreal.MaterialExpressionSaturate, 40, -260)
mel.connect_material_expressions(center_value, "", center_hit, "")

outside_shape = expr(unreal.MaterialExpressionOneMinus, 220, -260)
mel.connect_material_expressions(center_hit, "", outside_shape, "")

outline = expr(unreal.MaterialExpressionMultiply, 400, -440)
mel.connect_material_expressions(neighbour_hit, "", outline, "A")
mel.connect_material_expressions(outside_shape, "", outline, "B")

# --- Color = the palette slot the stencil index names ---
outline_color = palette_slot(SLOTS[0], 0, 200)
for index, slot in enumerate(SLOTS[1:], start=1):
    row = 200 + index * 180
    slot_param = palette_slot(slot, 0, row)

    reaches_slot = expr(unreal.MaterialExpressionStep, 260, row + 60)
    reaches_slot.set_editor_property("const_y", float(index + 1))
    mel.connect_material_expressions(neighbour_value, "", reaches_slot, "X")

    blend = expr(unreal.MaterialExpressionLinearInterpolate, 520, row)
    mel.connect_material_expressions(outline_color, "", blend, "A")
    mel.connect_material_expressions(slot_param, "", blend, "B")
    mel.connect_material_expressions(reaches_slot, "", blend, "Alpha")
    outline_color = blend

# --- Result = lerp(scene, Color, Outline) ---
scene = expr(unreal.MaterialExpressionSceneTexture, 700, -300)
scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)

# SceneTexture Color and a collection parameter are both float4 — the lerp needs both sides at the
# width the emissive output takes, and the palette's authored alpha has no meaning for an outline.
scene_color = mask(scene, "Color", 880, -300, True, True, True)
outline_rgb = mask(outline_color, "", 780, 140, True, True, True)

result = expr(unreal.MaterialExpressionLinearInterpolate, 1060, -160)
mel.connect_material_expressions(scene_color, "", result, "A")
mel.connect_material_expressions(outline_rgb, "", result, "B")
mel.connect_material_expressions(outline, "", result, "Alpha")

mel.connect_material_property(result, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# --- settings (after wiring) ---
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_DOF)

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

unreal.log(f"OUTLINEMAT::built {FULL} + {INSTANCE_NAME} + {COLLECTION_NAME}")
