"""
The rich-text styles of every ability text (the character sheet's ability rows, the ability drawer), and the widget
helpers the other UI scripts share.
- DT_AbilityText / DT_AbilityMeta: rich-text styles of an ability's description and of its cooldown line (Default +
  Value rows), created once from the theme's Body and Label fonts; tune them on the tables.

Never overwrites a hand edit: the tables are built only when created, and every value goes through asset_guard.write
(kept values are listed in AI/Output/ability_page.txt).
Usage: run via MCP execute_script, after ui_theme.py and import_icons.py.
"""
import importlib
import json
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import asset_guard

asset_guard = importlib.reload(asset_guard)

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
ROLE = unreal.GeoTextRole
STYLE_DIR = "/Game/HUD/Style"
FONT_DIR = "/Game/HUD/Assets/Fonts"
MENU_DIR = "/Game/HUD/InGameMenu"
TEXT_TABLE_PATH = f"{STYLE_DIR}/DT_AbilityText"
META_TABLE_PATH = f"{STYLE_DIR}/DT_AbilityMeta"

VALUE_COLOR = "FFC840"
ICON_TILE_PADDING = unreal.Margin(12, 12, 12, 12)
KEY_PADDING = unreal.Margin(8, 1, 8, 1)


def srgb(hex_code, alpha=1.0):
    color = unreal.Color(r=int(hex_code[0:2], 16), g=int(hex_code[2:4], 16), b=int(hex_code[4:6], 16),
                         a=int(alpha * 255))
    linear = unreal.MathLibrary.conv_color_to_linear_color(color)
    linear.a = alpha
    return linear


# --- Rich-text style tables -------------------------------------------------------------------------------------------
def style_row_json(name, font_info, color):
    font_object = font_info.get_editor_property("font_object")
    return {
        "Name": name,
        "TextStyle": {
            "Font": {
                "FontObject": f"{font_object.get_class().get_path_name()}'{font_object.get_path_name()}'",
                "TypefaceFontName": str(font_info.get_editor_property("typeface_font_name")),
                "Size": font_info.get_editor_property("size"),
                "LetterSpacing": font_info.get_editor_property("letter_spacing"),
            },
            "ColorAndOpacity": {"SpecifiedColor": {"R": color.r, "G": color.g, "B": color.b, "A": color.a},
                                "ColorUseRule": "UseColor_Specified"},
        },
    }


def build_style_table(path, role):
    """A Default row in role's font and colour and a Value row in the value colour; only when the table is new."""
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    folder, name = path.rsplit("/", 1)
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.RichTextStyleRow.static_struct())
    table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.DataTable, factory)
    theme = unreal.load_asset(f"{STYLE_DIR}/DA_UITheme")
    text_style = theme.get_editor_property("text_styles")[role]
    font_info = text_style.get_editor_property("font")
    body_color = text_style.get_editor_property("color").get_editor_property("specified_color")
    rows = [style_row_json("Default", font_info, body_color), style_row_json("Value", font_info, srgb(VALUE_COLOR))]
    unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows))
    unreal.EditorAssetLibrary.save_loaded_asset(table)
    return table


# --- Widget helpers ---------------------------------------------------------------------------------------------------
def add(wbp, path, widget_class, name, parent, values=None):
    """Constructs `name` under parent when the tree has no widget of that name, then writes values through the guard.
    Returns the widget and its slot."""
    widget = UTIL.find_widget(wbp, name)
    if not widget:
        widget = UTIL.construct_widget_in_tree(wbp, widget_class, name, True)
        UTIL.attach_widget(wbp, parent, name)
    for key, value in (values or {}).items():
        asset_guard.write(path, widget, key, value)
    return widget, widget.get_editor_property("slot")


def write_slot(path, slot, values):
    for key, value in values.items():
        asset_guard.write(path, slot, key, value)


def frame_values(style_name, padding, hover=False):
    return {"frame_style": unreal.load_asset(f"{STYLE_DIR}/{style_name}"), "padding": padding,
            "activate_on_hover_and_focus": hover}


def run():
    build_style_table(TEXT_TABLE_PATH, ROLE.BODY)
    build_style_table(META_TABLE_PATH, ROLE.LABEL)
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "ability_page.txt")
    open(output, "w").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
