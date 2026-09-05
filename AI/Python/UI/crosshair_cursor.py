"""
Create the crosshair software-cursor widget and bind it to the Crosshairs cursor slot.
Specific-type caller: builds a luminance-masked UI material from the cursor texture (black -> transparent),
then composes a single-Image-root widget via the generic GeoWidgetBuilderUtil.SetImageRootFromMaterial.
Usage: run via MCP execute_script (editor running, build registering SetImageRootFromMaterial).
The Crosshairs binding itself lives in Config/DefaultEngine.ini (set with the editor closed).
"""
import unreal

CURSOR_TEXTURE_PATH = "/Engine/EditorMaterials/TargetIcon.TargetIcon"
MATERIAL_NAME = "M_CrosshairCursor"
WBP_NAME = "WBP_CrosshairCursor"
ASSET_FOLDER = "/Game/HUD/Cursor"
CURSOR_SIZE = unreal.Vector2D(32.0, 32.0)


def create_asset(name, folder, asset_class, factory):
    full_path = f"{folder}/{name}"
    existing = unreal.load_asset(full_path)
    if existing:
        return existing
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(name, folder, asset_class, factory)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def build_mask_material(texture):
    """UI material: Emissive = texture RGB, Opacity = max(R,G,B) so black pixels are transparent."""
    mat = create_asset(MATERIAL_NAME, ASSET_FOLDER, unreal.Material, unreal.MaterialFactoryNew())

    lib = unreal.MaterialEditingLibrary

    sample = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -400, 0)
    sample.set_editor_property("texture", texture)

    # Opacity = max(R, G) then max(that, B) — bright (white) lines opaque, black background transparent.
    max_rg = lib.create_material_expression(mat, unreal.MaterialExpressionMax, -150, 150)
    max_rgb = lib.create_material_expression(mat, unreal.MaterialExpressionMax, 0, 150)

    lib.connect_material_expressions(sample, "R", max_rg, "A")
    lib.connect_material_expressions(sample, "G", max_rg, "B")
    lib.connect_material_expressions(max_rg, "", max_rgb, "A")
    lib.connect_material_expressions(sample, "B", max_rgb, "B")

    lib.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.connect_material_property(max_rgb, "", unreal.MaterialProperty.MP_OPACITY)

    # UI domain + translucent unlit so the mask shows through as alpha.
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def create_widget_blueprint(name, folder):
    return create_asset(name, folder, unreal.WidgetBlueprint, unreal.WidgetBlueprintFactory())


def build_crosshair_cursor():
    texture = unreal.load_asset(CURSOR_TEXTURE_PATH)
    if not texture:
        unreal.log_error(f"crosshair_cursor.py: texture not found: {CURSOR_TEXTURE_PATH}")
        return

    material = build_mask_material(texture)
    wbp = create_widget_blueprint(WBP_NAME, ASSET_FOLDER)

    # Compose the cursor from the generic material-image primitive (typed caller supplies material + size).
    cdo = unreal.GeoWidgetBuilderUtil.get_default_object()
    cdo.set_image_root_from_material(wbp, material, CURSOR_SIZE)

    unreal.log("crosshair_cursor.py: done — WBP_CrosshairCursor built with masked material "
               "(Crosshairs binding is in DefaultEngine.ini).")


build_crosshair_cursor()
