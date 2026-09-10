"""
Build WBP_ListPanel — the frame every full-screen list wears — dressed in the background the menus themselves wear,
and re-root the server browser onto it. The leaderboard is put on the same frame by leaderboard_menu.py, which runs
after this.
Usage: run via MCP execute_script, AFTER list_row.py. Re-run-safe end to end. Adjust the example call at the bottom.
"""
import unreal

BUILDER = unreal.GeoWidgetBuilderUtil.get_default_object()
HUD_BUILDER = unreal.GeoHudWidgetBuilderUtil.get_default_object()


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
    BUILDER.inspect_widget_blueprint(wbp)


def set_cdo_property(wbp, prop_name, value):
    """Set a class default on a widget BP and save. Soft-object-pointer values must be LOADED assets."""
    unreal.get_default_object(wbp.generated_class()).set_editor_property(prop_name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)


def find_controls(wbp, panel_name):
    """The panel's first non-Image child — the controls the designer authored, told from the background behind them
    without depending on names they chose."""
    panel = BUILDER.find_widget(wbp, panel_name)
    if not panel:
        inspect_widget_tree(wbp)
        raise LookupError(f"{wbp.get_name()} holds no panel named '{panel_name}' — its tree is logged above")
    for child in panel.get_all_children():
        if not isinstance(child, unreal.Image):
            return child
    return None


def read_skin(wbp, image_name):
    """A background as plain values, so it dresses an image in another asset."""
    image = BUILDER.find_widget(wbp, image_name)
    if not image:
        inspect_widget_tree(wbp)
        raise LookupError(f"{wbp.get_name()} holds no image named '{image_name}' — its tree is logged above")
    return image.get_editor_property('brush'), image.get_editor_property('color_and_opacity')


def apply_skin(frame, target_name, skin):
    """Dress one of the built backgrounds, so the frame wears the authored panel rather than a white box."""
    target = BUILDER.find_widget(frame, target_name)
    target.set_editor_property('brush', skin[0])
    target.set_editor_property('color_and_opacity', skin[1])


def build_list_frame(frame_name, frame_folder, frame_class_path, skin_source_path, skin_image_name):
    """Build WBP_ListPanel wearing the background the menus themselves wear. Header strip and list page carry the one
    skin, so a tab sitting on the seam between them opens onto the page."""
    frame = create_widget_blueprint(frame_name, frame_folder, frame_class_path)
    skin = read_skin(unreal.load_asset(skin_source_path), skin_image_name)

    HUD_BUILDER.build_list_panel_widget(frame)

    apply_skin(frame, 'HeaderBackground', skin)
    apply_skin(frame, 'ContentBackground', skin)
    BUILDER.commit_tree(frame)

    inspect_widget_tree(frame)
    return frame


def descendants(widget):
    if not isinstance(widget, unreal.PanelWidget):
        return []
    found = []
    for child in widget.get_all_children():
        found.append(child)
        found.extend(descendants(child))
    return found


def wear_frame(panel, frame, header_content_name, footer_content_name, keep):
    """Re-root an authored panel onto the frame: its header controls and back button move into the frame's slots —
    names, GUIDs and graph wiring intact — and everything the old root still held goes away with it. Anything in
    keep that the header box does not already hold is pulled into it first, so no BindWidget is left behind."""
    BUILDER.construct_widget_in_tree(panel, frame.generated_class(), 'ListFrame', True)

    header = BUILDER.find_widget(panel, header_content_name)
    kept = {widget.get_name() for widget in descendants(header)}
    for name in keep:
        if name not in kept and name != footer_content_name:
            BUILDER.attach_widget(panel, header_content_name, name, -1)

    BUILDER.set_named_slot_content(panel, 'ListFrame', 'HeaderSlot', header_content_name)
    BUILDER.set_named_slot_content(panel, 'ListFrame', 'FooterSlot', footer_content_name)
    BUILDER.set_root_widget(panel, 'ListFrame')
    BUILDER.commit_tree(panel)
    inspect_widget_tree(panel)


# --- Example: the GeoTrinity shared list frame ---
shared_frame = build_list_frame(
    frame_name='WBP_ListPanel',
    frame_folder='/Game/HUD',
    frame_class_path='/Script/GeoTrinityUI.GeoListFrameWidget',
    skin_source_path='/Game/HUD/MainMenu/WBP_CreateServerWidget',
    skin_image_name='Image_MenuBackground',
)

browse_servers = unreal.load_asset('/Game/HUD/MainMenu/WBP_BrowseServers')

# Only on the first run: once the browser wears the frame, its own header/content panels are gone and its controls
# already sit in the frame's slots, which a rebuild of the frame leaves alone.
if BUILDER.find_widget(browse_servers, 'OV_Header'):
    wear_frame(
        panel=browse_servers,
        frame=shared_frame,
        # The controls the designer authored in the header, whatever box they sit in.
        header_content_name=find_controls(browse_servers, 'OV_Header').get_name(),
        footer_content_name='BackButton',
        keep=['SearchInput', 'LanguageComboBox', 'SearchProgressBar', 'RefreshButton'],
    )

set_cdo_property(browse_servers, 'RowWidgetClass', unreal.load_asset('/Game/HUD/WBP_ListRow').generated_class())
