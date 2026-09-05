"""
Set up a child panel widget inside an existing menu widget BP: create the panel WBP, build its tree via a
C++ shim, set class defaults, inject into the parent menu, and apply per-template properties (a child widget
template does NOT inherit CDO defaults set afterwards).
Usage: run via MCP execute_script. Re-run-safe end to end. Adjust the example call at the bottom.
"""
import unreal


def create_widget_blueprint(name, folder, parent_class_path):
    full_path = f"{folder}/{name}"
    existing = unreal.load_asset(full_path)
    if existing:
        return existing
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, parent_class_path))
    wbp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.WidgetBlueprint, factory)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)
    return wbp


def inspect_widget_tree(wbp):
    """Log a widget BP's full tree (types, names, slots) — read it back via the output-log tool."""
    unreal.GeoWidgetBuilderUtil.get_default_object().inspect_widget_blueprint(wbp)


def set_cdo_property(wbp, prop_name, value):
    """Set a class default on a widget BP and save. Soft-object-pointer values must be LOADED assets."""
    unreal.get_default_object(wbp.generated_class()).set_editor_property(prop_name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)


def set_child_template_property(parent_wbp, child_class_name, prop_name, value):
    """
    Set a property on a child user-widget TEMPLATE stored inside a parent widget BP's tree.
    Required because templates snapshot their values: later edits to the child class CDO never reach them.
    Skips CDOs and live transient (PIE) instances; saves the parent asset.
    """
    for obj in unreal.ObjectIterator():
        try:
            if (obj.get_class().get_name() == child_class_name
                    and 'Default__' not in obj.get_name()
                    and 'Transient' not in obj.get_path_name()):
                obj.set_editor_property(prop_name, value)
        except Exception:
            pass
    unreal.EditorAssetLibrary.save_loaded_asset(parent_wbp)


def setup_local_connect_menu(panel_name, panel_folder, panel_parent_class_path, button_class_path,
                             main_menu_path, canvas_name, buttons_box_name, host_map_path):
    panel = create_widget_blueprint(panel_name, panel_folder, panel_parent_class_path)

    button_class = unreal.load_class(None, button_class_path)
    util = unreal.GeoHudWidgetBuilderUtil.get_default_object()
    util.build_local_connect_widget(panel, button_class)

    host_map = unreal.load_asset(host_map_path)
    set_cdo_property(panel, 'HostMap', host_map)

    main_menu = unreal.load_asset(main_menu_path)
    util.add_local_connect_to_main_menu(main_menu, canvas_name, buttons_box_name,
                                        button_class, panel.generated_class())

    # The injected template snapshots its values independently of the CDO — set per-template too.
    set_child_template_property(main_menu, f'{panel_name}_C', 'HostMap', host_map)

    inspect_widget_tree(main_menu)


# --- Example: the GeoTrinity Play Local panel ---
setup_local_connect_menu(
    panel_name='WBP_LocalConnect',
    panel_folder='/Game/HUD/MainMenu',
    panel_parent_class_path='/Script/GeoTrinity.GeoLocalConnectWidget',
    button_class_path='/Game/HUD/WBP_GeoButton.WBP_GeoButton_C',
    main_menu_path='/Game/HUD/MainMenu/WBP_MainMenuWidget',
    canvas_name='CanvasPanel_0',
    buttons_box_name='VB_MainButtons',
    host_map_path='/Game/Maps/DraftMap',
)
