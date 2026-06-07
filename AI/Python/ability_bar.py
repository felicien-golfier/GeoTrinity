"""
Ability bar HUD asset pipeline. Run via MCP execute_script (editor running, ability-bar build registered).

Builds, in order:
  1. M_CooldownSweep  — UI material, radial sweep masked by a "Fill" scalar (0=ready, 1=full cooldown).
  2. WBP_AbilitySlot  — slot widget tree via the GeoHudWidgetBuilderUtil.BuildAbilitySlotWidget shim.
  3. WBP_AbilityBar   — bar widget: Overlay root with a centered "SlotBox" HorizontalBox (BuildAbilityBarWidget shim).

Wiring of the bar into the player overlay + HUD BP InitAbilityBar/RefreshAbilityBar events is done separately
(those touch BP event graphs). Icon + bShowDeployCount population is in a separate step.
"""
import unreal
import math

ASSET_FOLDER = "/Game/HUD/AbilityBar"
MATERIAL_NAME = "M_CooldownSweep"
SLOT_WBP_NAME = "WBP_AbilitySlot"
BAR_WBP_NAME = "WBP_AbilityBar"
SLOT_SQUARE_SIZE = 64.0


def create_asset(name, folder, asset_class, factory):
    full_path = f"{folder}/{name}"
    existing = unreal.load_asset(full_path)
    if existing:
        return existing
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(name, folder, asset_class, factory)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def build_cooldown_sweep_material():
    """UI material: a dark wedge covering the [0, Fill] angular range over the icon.
    Emissive = dark tint; Opacity = Step(angle01, Fill) * BaseAlpha so the swept wedge dims the icon."""
    mat = create_asset(MATERIAL_NAME, ASSET_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    lib = unreal.MaterialEditingLibrary

    # uvn = (UV - 0.5) * 2  -> centered [-1, 1]
    tex_coord = lib.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    sub = lib.create_material_expression(mat, unreal.MaterialExpressionSubtract, -700, 0)
    sub.set_editor_property("const_b", 0.5)
    mul2 = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -550, 0)
    mul2.set_editor_property("const_b", 2.0)
    lib.connect_material_expressions(tex_coord, "", sub, "A")
    lib.connect_material_expressions(sub, "", mul2, "A")

    # angle = atan2(uvn.y, uvn.x); normalize to [0,1] via /(2pi) + 0.5
    comp = lib.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -400, 0)
    comp.set_editor_property("r", True)
    comp.set_editor_property("g", True)
    comp.set_editor_property("b", False)
    comp.set_editor_property("a", False)
    lib.connect_material_expressions(mul2, "", comp, "")

    atan = lib.create_material_expression(mat, unreal.MaterialExpressionArctangent2, -250, 0)
    # Arctangent2 input pins are named "Y" and "X" (atan2(Y, X)). Mask gives (x,y); feed x->X, y->Y via further masks.
    mask_x = lib.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -380, 150)
    mask_x.set_editor_property("r", True)
    mask_x.set_editor_property("g", False)
    mask_x.set_editor_property("b", False)
    mask_x.set_editor_property("a", False)
    mask_y = lib.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -380, 280)
    mask_y.set_editor_property("r", False)
    mask_y.set_editor_property("g", True)
    mask_y.set_editor_property("b", False)
    mask_y.set_editor_property("a", False)
    lib.connect_material_expressions(mul2, "", mask_x, "")
    lib.connect_material_expressions(mul2, "", mask_y, "")
    lib.connect_material_expressions(mask_y, "", atan, "Y")
    lib.connect_material_expressions(mask_x, "", atan, "X")

    # normalize: angle / (2pi) + 0.5
    norm = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -80, 0)
    norm.set_editor_property("const_b", 1.0 / (2.0 * math.pi))
    add = lib.create_material_expression(mat, unreal.MaterialExpressionAdd, 60, 0)
    add.set_editor_property("const_b", 0.5)
    lib.connect_material_expressions(atan, "", norm, "A")
    lib.connect_material_expressions(norm, "", add, "A")

    # Fill scalar param (0 = ready/no wedge, 1 = full wedge)
    fill = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 60, 200)
    fill.set_editor_property("parameter_name", "Fill")
    fill.set_editor_property("default_value", 0.0)

    # wedge = Step(angle01, Fill) -> 1 where Fill >= angle01
    step = lib.create_material_expression(mat, unreal.MaterialExpressionStep, 250, 100)
    lib.connect_material_expressions(add, "", step, "Y")
    lib.connect_material_expressions(fill, "", step, "X")

    # Opacity = wedge * BaseAlpha
    base_alpha = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, 250, 280)
    base_alpha.set_editor_property("r", 0.6)
    opacity = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, 450, 150)
    lib.connect_material_expressions(step, "", opacity, "A")
    lib.connect_material_expressions(base_alpha, "", opacity, "B")

    # Emissive = dark tint
    tint = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, 450, 320)
    tint.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))

    lib.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def build_slot_widget():
    wbp = create_asset(SLOT_WBP_NAME, ASSET_FOLDER, unreal.WidgetBlueprint, _slot_factory())
    cdo = unreal.GeoHudWidgetBuilderUtil.get_default_object()
    cdo.build_ability_slot_widget(wbp, SLOT_SQUARE_SIZE)
    return wbp


def build_bar_widget():
    wbp = create_asset(BAR_WBP_NAME, ASSET_FOLDER, unreal.WidgetBlueprint, _bar_factory())
    cdo = unreal.GeoHudWidgetBuilderUtil.get_default_object()
    cdo.build_ability_bar_widget(wbp)
    return wbp


def _slot_factory():
    f = unreal.WidgetBlueprintFactory()
    f.set_editor_property("parent_class", unreal.load_class(None, "/Script/GeoTrinity.GeoAbilitySlotWidget"))
    return f


def _bar_factory():
    f = unreal.WidgetBlueprintFactory()
    f.set_editor_property("parent_class", unreal.load_class(None, "/Script/GeoTrinity.GeoAbilityBarWidget"))
    return f


def run():
    build_cooldown_sweep_material()
    build_slot_widget()
    build_bar_widget()
    unreal.log("ability_bar.py: done — M_CooldownSweep, WBP_AbilitySlot, WBP_AbilityBar built.")


run()
