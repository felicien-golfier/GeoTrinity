"""
Dress the shared menu widgets in the Rail look: every one wears a UGeoFrame whose style asset (/Game/HUD/Style) carries
the look, so tuning a style asset re-skins every widget wearing it. Structure only lives here; colours, fonts and
motion live in the style assets and DA_UITheme (built by ui_theme.py, which must run first).
- WBP_GeoButton: Frame (button style, active on hover/focus) > ButtonWidget > ButtonText.
- WBP_ListRow: RowFrame (row style) > RowButton > ColumnsBox.
- WBP_ListPanel: a panel frame behind its header and rows, the old background images dropped.
Usage: run via MCP execute_script. Rebuilds whole widgets, so it refuses an existing asset (asset_guard.legacy_rebuild)
until converted to asset_guard.write; clickable_frames(), which runs first, is converted and safe on existing assets.
"""
import importlib
import os
import sys

import unreal

_UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if _UI_SCRIPTS not in sys.path:
    sys.path.insert(0, _UI_SCRIPTS)
import asset_guard

asset_guard = importlib.reload(asset_guard)

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
STYLE_DIR = "/Game/HUD/Style"

BUTTON_PATH = "/Game/HUD/WBP_GeoButton"
ROW_PATH = "/Game/HUD/WBP_ListRow"
LIST_PANEL_PATH = "/Game/HUD/WBP_ListPanel"

# A frame holds its button with no padding, so the whole outline it draws is clickable: the room round a label or a
# row's columns lives inside the button. A frame padding would be outline the button does not cover, and a button
# squeezed below its frame's padding (a narrow - or +) would have nothing left to click.
NO_PADDING = unreal.Margin(0, 0, 0, 0)
BUTTON_PADDING = unreal.Margin(36, 14, 36, 14)
ROW_PADDING = unreal.Margin(14, 6, 14, 6)
# What the frames held before, over the button slot's own 4, 2.
EARLIER_FRAME_PADDING = {BUTTON_PATH: unreal.Margin(32, 12, 32, 12), ROW_PATH: unreal.Margin(10, 4, 10, 4)}
PANEL_PADDING = unreal.Margin(28, 24, 28, 24)
FOOTER_GAP = 16
ROW_TINTS = {
    "normal_color": unreal.LinearColor(1, 1, 1, 0),
    "alternate_color": unreal.LinearColor(1, 1, 1, .025),
    "header_color": unreal.LinearColor(1, 1, 1, 0),
    "selected_color": unreal.LinearColor(.45, .2, 1, .08),
}


def frame_style(name):
    return unreal.load_asset(f"{STYLE_DIR}/{name}")


def no_draw_brush():
    brush = unreal.SlateBrush()
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
    return brush


def white_box_brush():
    brush = unreal.SlateBrush()
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
    return brush


def clear_button_style(button, normal=None):
    """A button that draws nothing itself: the frame around it is its look. Normal keeps a brush when given, for a row
    whose code tints it."""
    style = button.get_editor_property("widget_style").copy()
    for state in ["normal", "hovered", "pressed", "disabled"]:
        style.set_editor_property(state, no_draw_brush())
    if normal:
        style.set_editor_property("normal", normal)
    for padding in ["normal_padding", "pressed_padding"]:
        style.set_editor_property(padding, unreal.Margin(0, 0, 0, 0))
    button.set_editor_property("widget_style", style)


# Properties carried over when a widget is swapped for its themed class, read with try so each type keeps its own.
CARRIED_WIDGET_PROPERTIES = [
    "visibility", "is_enabled", "tool_tip_text", "render_opacity",
    "text", "justification", "auto_wrap_text", "hint_text", "is_password", "is_read_only",
    "select_all_text_when_focused", "clear_keyboard_focus_on_commit", "minimum_desired_width",
    "default_options", "selected_option", "max_list_height", "has_down_arrow", "enable_gamepad_navigation_mode",
    "checked_state", "value", "min_value", "max_value", "step_size", "orientation", "mouse_uses_step",
    "percent", "bar_fill_type", "is_marquee",
]
CARRIED_SLOT_PROPERTIES = ["layout_data", "auto_size", "z_order", "padding", "size", "horizontal_alignment",
                           "vertical_alignment"]


def read_properties(obj, names):
    values = {}
    for name in names:
        try:
            values[name] = obj.get_editor_property(name)
        except Exception:
            pass
    return values


def write_properties(obj, values):
    for name, value in values.items():
        try:
            obj.set_editor_property(name, value)
        except Exception:
            pass


def swap_widget_class(wbp, name, new_class):
    """Replace the widget `name` by one of new_class (a themed subclass), keeping its name, place in its parent, slot
    layout and authored values, so BindWidgets and graph nodes naming it still resolve. Does not commit."""
    old = UTIL.find_widget(wbp, name)
    if not old or old.get_class() == new_class.static_class():
        return old
    parent = old.get_parent()
    index = parent.get_child_index(old)
    widget_values = read_properties(old, CARRIED_WIDGET_PROPERTIES)
    slot_values = read_properties(old.get_editor_property("slot"), CARRIED_SLOT_PROPERTIES)
    children = []
    if isinstance(old, unreal.PanelWidget):
        children = [(child.get_name(), read_properties(child.get_editor_property("slot"), CARRIED_SLOT_PROPERTIES))
                    for child in old.get_all_children()]

    widget = UTIL.construct_widget_in_tree(wbp, new_class, name, True)
    slot = UTIL.attach_widget(wbp, parent.get_name(), name, index)
    write_properties(widget, widget_values)
    write_properties(slot, slot_values)
    for child_name, child_slot_values in children:
        write_properties(UTIL.attach_widget(wbp, name, child_name), child_slot_values)
    return widget


def reload_blueprint(wbp):
    """Save, reload and recompile: graph nodes reading a swapped widget keep its old pin type until the package is
    reloaded, and the compile in between reports the mismatch. Returns the reloaded asset."""
    path = wbp.get_path_name().split(".")[0]
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)
    unreal.EditorLoadingAndSavingUtils.reload_packages([wbp.get_outermost()])
    wbp = unreal.load_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)
    return wbp


# The themed class each engine widget is swapped for; their look then comes from DA_UITheme.
THEMED_CLASSES = {
    "TextBlock": "GeoText",
    "EditableTextBox": "GeoEditableTextBox",
    "ComboBoxString": "GeoComboBoxString",
    "CheckBox": "GeoCheckBox",
    "Slider": "GeoSlider",
    "ProgressBar": "GeoProgressBar",
    "ScrollBox": "GeoScrollBox",
}


def theme_widget(wbp, name, role=None):
    """Swap the widget `name` for its themed class; a text also takes `role`. Does not commit."""
    widget = UTIL.find_widget(wbp, name)
    themed_name = THEMED_CLASSES.get(widget.get_class().get_name())
    if themed_name:
        widget = swap_widget_class(wbp, name, getattr(unreal, themed_name))
    if role is not None:
        widget.set_editor_property("role", role)
    return widget


def wrap_in_frame(wbp, name, frame_name, style_name, padding=unreal.Margin(0, 0, 0, 0),
                  activate_on_hover_and_focus=True):
    """Put the widget `name` inside a new UGeoFrame wearing style_name, which takes its place and slot layout in the
    parent. Re-run-safe: an existing frame of that name is unwrapped first. Does not commit."""
    existing = UTIL.find_widget(wbp, frame_name)
    if existing and existing.get_parent():
        existing_parent = existing.get_parent().get_name()
        index = existing.get_parent().get_child_index(existing)
        slot_values = read_properties(existing.get_editor_property("slot"), CARRIED_SLOT_PROPERTIES)
        UTIL.attach_widget(wbp, existing_parent, name, index)
        write_properties(UTIL.find_widget(wbp, name).get_editor_property("slot"), slot_values)
        UTIL.remove_widget(wbp, frame_name)

    widget = UTIL.find_widget(wbp, name)
    parent = widget.get_parent()
    index = parent.get_child_index(widget)
    slot_values = read_properties(widget.get_editor_property("slot"), CARRIED_SLOT_PROPERTIES)

    frame = UTIL.construct_widget_in_tree(wbp, unreal.GeoFrame, frame_name, True)
    frame.set_editor_property("frame_style", frame_style(style_name))
    frame.set_editor_property("activate_on_hover_and_focus", activate_on_hover_and_focus)
    frame.set_editor_property("padding", padding)
    write_properties(UTIL.attach_widget(wbp, parent.get_name(), frame_name, index), slot_values)
    fill_slot(UTIL.attach_widget(wbp, frame_name, name))
    return frame


def add_backdrop_frame(wbp, overlay_name, frame_name, style_name):
    """A UGeoFrame filling the overlay `overlay_name` behind its other children: the plate a panel sits on. Does not
    commit."""
    frame = UTIL.construct_widget_in_tree(wbp, unreal.GeoFrame, frame_name, True)
    frame.set_editor_property("frame_style", frame_style(style_name))
    frame.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    fill_slot(UTIL.attach_widget(wbp, overlay_name, frame_name, 0))
    return frame


def make_frame_root(wbp, name, style_name, padding, activate_on_hover_and_focus):
    UTIL.set_root_panel(wbp, unreal.GeoFrame, name)
    frame = UTIL.find_widget(wbp, name)
    frame.set_editor_property("frame_style", frame_style(style_name))
    frame.set_editor_property("activate_on_hover_and_focus", activate_on_hover_and_focus)
    frame.set_editor_property("padding", padding)
    return frame


def construct_into(wbp, widget_class, name, parent_name):
    widget = UTIL.construct_widget_in_tree(wbp, widget_class, name, True)
    slot = UTIL.attach_widget(wbp, parent_name, name)
    return widget, slot


def fill_slot(slot):
    slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_FILL)


def style_menu_button():
    """Frame > ButtonWidget > ButtonText. The label's font comes from the button's TextRole in the theme."""
    wbp = asset_guard.legacy_rebuild(BUTTON_PATH)
    make_frame_root(wbp, "Frame", "DA_Frame_Button", NO_PADDING, True)

    button, slot = construct_into(wbp, unreal.GeoButton, "ButtonWidget", "Frame")
    fill_slot(slot)
    clear_button_style(button)

    text, text_slot = construct_into(wbp, unreal.TextBlock, "ButtonText", "ButtonWidget")
    text.set_editor_property("text", unreal.Text("BUTTON"))
    text.set_editor_property("justification", unreal.TextJustify.CENTER)
    text_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
    text_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    text_slot.set_editor_property("padding", BUTTON_PADDING)
    UTIL.commit_tree(wbp)


def style_list_row():
    """RowFrame > RowButton > ColumnsBox. The row's code tints the button's normal brush per row role."""
    wbp = asset_guard.legacy_rebuild(ROW_PATH)
    make_frame_root(wbp, "RowFrame", "DA_Frame_Row", NO_PADDING, True)

    button, slot = construct_into(wbp, unreal.GeoButton, "RowButton", "RowFrame")
    fill_slot(slot)
    clear_button_style(button, normal=white_box_brush())

    _, columns_slot = construct_into(wbp, unreal.HorizontalBox, "ColumnsBox", "RowButton")
    fill_slot(columns_slot)
    columns_slot.set_editor_property("padding", ROW_PADDING)
    UTIL.commit_tree(wbp)
    seed_row_tints(wbp)


def seed_row_tints(wbp):
    """Rows sit on the panel's dark plate: no tint but a faint stripe and a violet wash on the selected row. Only tints
    still at the class's own seed are written, so a tint tuned on the asset survives a re-run."""
    native = unreal.get_default_object(unreal.load_class(None, "/Script/GeoTrinityUI.GeoListRowWidget"))
    row = unreal.get_default_object(wbp.generated_class())
    for name, value in ROW_TINTS.items():
        if row.get_editor_property(name) == native.get_editor_property(name):
            row.set_editor_property(name, value)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)


def style_list_panel():
    """The panel frame fills the panel behind the header strip and the rows; the backgrounds they wore are dropped."""
    wbp = asset_guard.legacy_rebuild(LIST_PANEL_PATH)
    for image in ["HeaderBackground", "ContentBackground"]:
        if UTIL.find_widget(wbp, image):
            UTIL.remove_widget(wbp, image)

    frame = UTIL.construct_widget_in_tree(wbp, unreal.GeoFrame, "PanelFrame", True)
    frame.set_editor_property("frame_style", frame_style("DA_Frame_Panel"))
    frame.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    fill_slot(UTIL.attach_widget(wbp, "Root", "PanelFrame", 0))

    frame_box_slot = UTIL.find_widget(wbp, "FrameBox").get_editor_property("slot")
    frame_box_slot.set_editor_property("padding", PANEL_PADDING)
    theme_widget(wbp, "RowsBox")
    place_footer_under_rows(wbp)
    UTIL.commit_tree(wbp)


def place_footer_under_rows(wbp):
    """The footer (the back button) gets its own line under the rows, so it never covers the last one. Re-run-safe."""
    footer_slot = UTIL.attach_widget(wbp, "FrameBox", "FooterSlot")
    footer_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_RIGHT)
    footer_slot.set_editor_property("padding", unreal.Margin(0, FOOTER_GAP, 0, 0))
    footer_slot.set_editor_property("size", unreal.SlateChildSize(1.0, unreal.SlateSizeRule.AUTOMATIC))
    rows_slot = UTIL.find_widget(wbp, "ContentArea").get_editor_property("slot")
    rows_slot.set_editor_property("size", unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL))


def clickable_frames():
    """On a button and a list row built while their frame held the padding: the padding moves into the button, so the
    whole outline is clickable. Goes through asset_guard, so a frame padding changed by hand is kept and reported."""
    for path, frame_name, content_name, padding in [(BUTTON_PATH, "Frame", "ButtonText", BUTTON_PADDING),
                                                    (ROW_PATH, "RowFrame", "ColumnsBox", ROW_PADDING)]:
        wbp = unreal.load_asset(path)
        if asset_guard.write(path, UTIL.find_widget(wbp, frame_name), "padding", NO_PADDING,
                             earlier=(EARLIER_FRAME_PADDING[path],)):
            asset_guard.write(path, UTIL.find_widget(wbp, content_name).get_editor_property("slot"), "padding",
                              padding)
        UTIL.commit_tree(wbp)


if __name__ == "__main__":
    clickable_frames()
    style_menu_button()
    style_list_row()
    style_list_panel()
    for path in [BUTTON_PATH, ROW_PATH, LIST_PANEL_PATH]:
        UTIL.inspect_widget_blueprint(unreal.load_asset(path))
