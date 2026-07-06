"""
Generic helper for building a centered VerticalBox menu of labeled UGeoMenuButton rows (plus optional plain
UMG widgets) purely by composing the existing GeoWidgetBuilderUtil tree primitives — reusable for ANY future
menu screen, not just this one. Then uses it to create the in-game pause menu + settings panel widget
Blueprints, and wires the new Blueprint defaults on BP_GeoPlayerController and BP_GeoGameInstance.
Usage: run via MCP execute_script. Re-run-safe end to end.
"""
import unreal

util = unreal.GeoWidgetBuilderUtil


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


def set_cdo_property(wbp, prop_name, value):
    unreal.get_default_object(wbp.generated_class()).set_editor_property(prop_name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)


def build_centered_menu(wbp, root_name, box_name, rows, root_panel_class=unreal.VerticalBox):
    """
    Generic, reusable menu-tree builder: constructs root_panel_class as the tree root, then for each row in
    `rows` constructs and attaches a widget of the given class/name, applying any extra properties.
    rows: list of dicts, each either
      {'name': str, 'widget_class': UClass, 'props': {prop_name: value, ...}}
    Returns the root panel and box widgets (already committed/saved).
    Composable for any menu — pause menu, settings, future screens — by passing a different rows list.
    """
    util.set_root_panel(wbp, root_panel_class, root_name)
    box = None
    if root_name != box_name:
        box = util.construct_widget_in_tree(wbp, unreal.VerticalBox, box_name, True)
        util.attach_widget(wbp, root_name, box_name, -1)
        parent_name = box_name
    else:
        parent_name = root_name

    for row in rows:
        widget = util.construct_widget_in_tree(wbp, row['widget_class'], row['name'], True)
        util.attach_widget(wbp, parent_name, row['name'], -1)
        for prop_name, value in row.get('props', {}).items():
            widget.set_editor_property(prop_name, value)

    util.commit_tree(wbp)


def setup_pause_menu(settings_name, settings_folder, pause_name, pause_folder, button_class_path,
                     main_menu_map_path):
    button_class = unreal.load_class(None, button_class_path)

    def menu_button(name, label):
        return {'name': name, 'widget_class': button_class, 'props': {'Label': unreal.Text(label)}}

    def text_block(name, text):
        return {'name': name, 'widget_class': unreal.TextBlock, 'props': {'Text': unreal.Text(text)}}

    settings_wbp = create_widget_blueprint(settings_name, settings_folder,
                                           '/Script/GeoTrinityUI.GeoSettingsWidget')
    build_centered_menu(settings_wbp, 'Root', 'Root', [
        text_block('SoundLabel', 'Sound'),
        {'name': 'MasterVolumeSlider', 'widget_class': unreal.Slider, 'props': {'Value': 1.0}},
        text_block('KeyBindingsLabel', 'Key Bindings'),
        {'name': 'KeyBindingsList', 'widget_class': unreal.ScrollBox, 'props': {}},
        menu_button('BackButton', 'Back'),
    ])
    unreal.EditorAssetLibrary.save_loaded_asset(settings_wbp)

    pause_wbp = create_widget_blueprint(pause_name, pause_folder, '/Script/GeoTrinityUI.GeoPauseMenuWidget')
    build_centered_menu(pause_wbp, 'Root', 'Root', [
        menu_button('ResumeButton', 'Resume'),
        menu_button('SettingsButton', 'Settings'),
        menu_button('ReturnToMainMenuButton', 'Return to Main Menu'),
        {'name': 'SettingsWidget', 'widget_class': settings_wbp.generated_class(),
         'props': {'Visibility': unreal.SlateVisibility.COLLAPSED}},
    ])
    unreal.EditorAssetLibrary.save_loaded_asset(pause_wbp)

    # Wire BP_GeoPlayerController defaults: PauseMenuWidgetClass.
    pc_bp = unreal.load_asset('/Game/Characters/Playable/BP_GeoPlayerController')
    set_cdo_property(pc_bp, 'PauseMenuWidgetClass', pause_wbp.generated_class())

    # Wire BP_GeoGameInstance defaults: MainMenuMap.
    gi_bp = unreal.load_asset('/Game/Game/BP_GeoGameInstance')
    main_menu_map = unreal.load_asset(main_menu_map_path)
    set_cdo_property(gi_bp, 'MainMenuMap', main_menu_map)

    util.inspect_widget_blueprint(pause_wbp)
    util.inspect_widget_blueprint(settings_wbp)


# --- Example: GeoTrinity pause menu + settings panel ---
setup_pause_menu(
    settings_name='WBP_Settings',
    settings_folder='/Game/HUD/MainMenu',
    pause_name='WBP_PauseMenu',
    pause_folder='/Game/HUD/MainMenu',
    button_class_path='/Game/HUD/WBP_GeoButton.WBP_GeoButton_C',
    main_menu_map_path='/Game/Maps/MainMenu',
)
