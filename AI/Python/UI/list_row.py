"""
Turn the server-browser row into the row every list is built from: rebuild it as RowButton + ColumnsBox, keep its
authored skin, move it out of MainMenu, and point the server browser and the leaderboard at it.
Usage: run via MCP execute_script. Re-run-safe end to end. Adjust the example call at the bottom.
"""
import unreal


def inspect_widget_tree(wbp):
    """Log a widget BP's full tree (types, names, slots) — read it back via the output-log tool."""
    unreal.GeoWidgetBuilderUtil.get_default_object().inspect_widget_blueprint(wbp)


def set_cdo_property(wbp, prop_name, value):
    """Set a class default on a widget BP and save. Soft-object-pointer values must be LOADED assets."""
    unreal.get_default_object(wbp.generated_class()).set_editor_property(prop_name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)


def take_class_defaults(wbp, class_path, prop_names):
    """Properties the asset pinned before the class re-specified them, taken back from the class."""
    native = unreal.get_default_object(unreal.load_class(None, class_path))
    for name in prop_names:
        set_cdo_property(wbp, name, native.get_editor_property(name))


def read_widget_property(wbp, widget_name, prop_name):
    """A property of a widget that may no longer be in the tree — None once the row has been rebuilt."""
    widget = unreal.GeoWidgetBuilderUtil.get_default_object().find_widget(wbp, widget_name)
    if not widget:
        return None
    return widget.get_editor_property(prop_name)


def migrate_list_row(old_path, new_path, row_class_path, text_widget_name):
    """Rebuild the row and carry its authored button style and font over to the rebuilt widgets."""
    if unreal.EditorAssetLibrary.does_asset_exist(old_path):
        unreal.EditorAssetLibrary.rename_asset(old_path, new_path)

    row = unreal.load_asset(new_path)

    # Read the skin off the authored widgets before the rebuild discards them.
    button_style = read_widget_property(row, 'RowButton', 'widget_style')
    column_font = read_widget_property(row, text_widget_name, 'font')
    column_color = read_widget_property(row, text_widget_name, 'color_and_opacity')

    unreal.BlueprintEditorLibrary.reparent_blueprint(row, unreal.load_class(None, row_class_path))
    unreal.GeoHudWidgetBuilderUtil.get_default_object().build_list_row_widget(row)

    if button_style:
        unreal.GeoWidgetBuilderUtil.get_default_object().find_widget(row, 'RowButton').set_editor_property(
            'widget_style', button_style)
        unreal.GeoWidgetBuilderUtil.get_default_object().commit_tree(row)
    if column_font:
        set_cdo_property(row, 'ColumnFont', column_font)
    if column_color:
        set_cdo_property(row, 'ColumnColor', column_color)
    take_class_defaults(row, row_class_path, ['NormalColor', 'AlternateColor', 'HeaderColor', 'SelectedColor'])

    inspect_widget_tree(row)
    return row


def point_lists_at_row(row, list_paths):
    """Every list builds its rows from the one row class, so all of them are skinned by that one asset."""
    for path in list_paths:
        set_cdo_property(unreal.load_asset(path), 'RowWidgetClass', row.generated_class())


# --- Example: the GeoTrinity shared list row ---
shared_row = migrate_list_row(
    old_path='/Game/HUD/MainMenu/WBP_ServerRow',
    new_path='/Game/HUD/WBP_ListRow',
    row_class_path='/Script/GeoTrinityUI.GeoListRowWidget',
    text_widget_name='ServerNameText',
)

point_lists_at_row(shared_row, [
    '/Game/HUD/MainMenu/WBP_BrowseServers',
    '/Game/HUD/MainMenu/WBP_Leaderboard',
])
