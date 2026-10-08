"""
The abilities page in the Rail look: a header (title, the class shape, name and role, a hint, BACK) over a grid of
ability cards.
- DT_AbilityText / DT_AbilityMeta: rich-text styles of a card's description and of its cooldown line (Default + Value
  rows), created once from the theme's Body and Label fonts; tune them on the tables.
- WBP_AbilityCard (UGeoAbilityCardWidget): CardFrame > icon tile, name, key cap and timing, description, Reload buff
  lines, and the "see it in action" corner, a WIP zone for the coming spell clips.
- WBP_AbilityDescriptions: the header and CardGrid (inside the existing AbilityList scroll box); CardClass set.

Never overwrites a hand edit: the card and the tables are built only when created, widgets are added only when absent,
and every value goes through asset_guard.write (kept values are listed in AI/Output/ability_page.txt).
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
CARD_PATH = f"{MENU_DIR}/WBP_AbilityCard"
PAGE_PATH = f"{MENU_DIR}/WBP_AbilityDescriptions"
TEXT_TABLE_PATH = f"{STYLE_DIR}/DT_AbilityText"
META_TABLE_PATH = f"{STYLE_DIR}/DT_AbilityMeta"

VALUE_COLOR = "FFC840"
CARD_PADDING = unreal.Margin(20, 18, 20, 18)
ICON_TILE_PADDING = unreal.Margin(12, 12, 12, 12)
ICON_SIZE = 36.0
KEY_PADDING = unreal.Margin(8, 1, 8, 1)
GRID_GAP = unreal.Margin(10, 10, 10, 10)
CLASS_SHAPE_SIZE = 26.0


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


# --- Card -------------------------------------------------------------------------------------------------------------
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


def build_card(text_table, meta_table):
    if unreal.EditorAssetLibrary.does_asset_exist(CARD_PATH):
        return unreal.load_asset(CARD_PATH)
    asset_guard.created(CARD_PATH)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/GeoTrinityUI.GeoAbilityCardWidget"))
    folder, name = CARD_PATH.rsplit("/", 1)
    wbp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.WidgetBlueprint, factory)
    P = CARD_PATH
    fill = unreal.HorizontalAlignment.H_ALIGN_FILL

    UTIL.set_root_panel(wbp, unreal.GeoFrame, "CardFrame")
    for key, value in frame_values("DA_Frame_Card", CARD_PADDING, hover=True).items():
        asset_guard.write(P, UTIL.find_widget(wbp, "CardFrame"), key, value)
    add(wbp, P, unreal.VerticalBox, "CardBody", "CardFrame")

    add(wbp, P, unreal.HorizontalBox, "TopRow", "CardBody")
    _, tile_slot = add(wbp, P, unreal.GeoFrame, "IconFrame", "TopRow", frame_values("DA_Frame_IconTile", ICON_TILE_PADDING))
    write_slot(P, tile_slot, {"vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})
    add(wbp, P, unreal.GeoIconImage, "Icon", "IconFrame", {"size": ICON_SIZE})
    _, name_column_slot = add(wbp, P, unreal.VerticalBox, "NameColumn", "TopRow")
    write_slot(P, name_column_slot, {"padding": unreal.Margin(16, 0, 0, 0),
                                     "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})
    add(wbp, P, unreal.GeoText, "NameText", "NameColumn", {"role": ROLE.BUTTON, "text": unreal.Text("ABILITY")})
    _, meta_slot = add(wbp, P, unreal.HorizontalBox, "MetaRow", "NameColumn")
    write_slot(P, meta_slot, {"padding": unreal.Margin(0, 6, 0, 0)})
    _, key_slot = add(wbp, P, unreal.GeoFrame, "KeyFrame", "MetaRow", frame_values("DA_Frame_KeyCap", KEY_PADDING))
    write_slot(P, key_slot, {"vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})
    add(wbp, P, unreal.GeoText, "KeyText", "KeyFrame", {"role": ROLE.LABEL, "text": unreal.Text("KEY")})
    _, timing_slot = add(wbp, P, unreal.RichTextBlock, "TimingText", "MetaRow",
                         {"text_style_set": meta_table, "text": unreal.Text("COOLDOWN <Value>0s</>")})
    write_slot(P, timing_slot, {"padding": unreal.Margin(12, 0, 0, 0),
                                "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})

    _, description_slot = add(wbp, P, unreal.RichTextBlock, "DescriptionText", "CardBody",
                              {"text_style_set": text_table, "auto_wrap_text": True,
                               "text": unreal.Text("Description with <Value>values</>.")})
    write_slot(P, description_slot, {"padding": unreal.Margin(0, 14, 0, 0)})
    add(wbp, P, unreal.VerticalBox, "BuffBox", "CardBody")

    # The spell clip will live here; until then the corner only says it is coming.
    _, spacer_slot = add(wbp, P, unreal.Spacer, "ClipSpacer", "CardBody")
    write_slot(P, spacer_slot, {"size": unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)})
    _, wip_slot = add(wbp, P, unreal.GeoText, "ClipWipText", "CardBody",
                      {"role": ROLE.LABEL, "text": unreal.Text("SEE IT IN ACTION  ·  WIP"), "render_opacity": .55})
    write_slot(P, wip_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_RIGHT,
                             "padding": unreal.Margin(0, 14, 0, 0)})

    UTIL.commit_tree(wbp)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)
    return wbp


# --- Page -------------------------------------------------------------------------------------------------------------
def build_page(card):
    wbp = unreal.load_asset(PAGE_PATH)
    P = PAGE_PATH

    header = UTIL.find_widget(wbp, "Header")
    if not header:
        UTIL.construct_widget_in_tree(wbp, unreal.HorizontalBox, "Header", True)
        UTIL.attach_widget(wbp, "LayoutBox", "Header", 0)
    write_slot(P, UTIL.find_widget(wbp, "Header").get_editor_property("slot"), {"padding": unreal.Margin(0, 0, 0, 24)})
    _, title_slot = add(wbp, P, unreal.GeoText, "TitleText", "Header",
                        {"role": ROLE.HEADING, "text": unreal.Text("ABILITIES")})
    write_slot(P, title_slot, {"vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})
    _, shape_slot = add(wbp, P, unreal.GeoShape, "ClassShape", "Header",
                        {"size": CLASS_SHAPE_SIZE, "filled": True})
    write_slot(P, shape_slot, {"padding": unreal.Margin(32, 0, 12, 0),
                               "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})
    _, class_slot = add(wbp, P, unreal.GeoText, "ClassNameText", "Header",
                        {"role": ROLE.BUTTON, "text": unreal.Text("CLASS")})
    write_slot(P, class_slot, {"vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})
    _, role_slot = add(wbp, P, unreal.GeoText, "ClassRoleText", "Header",
                       {"role": ROLE.LABEL, "text": unreal.Text("ROLE")})
    write_slot(P, role_slot, {"padding": unreal.Margin(12, 0, 0, 0),
                              "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})
    _, hint_slot = add(wbp, P, unreal.GeoText, "HintText", "Header",
                       {"role": ROLE.LABEL, "text": unreal.Text("SPELL CLIPS COMING SOON"), "justification":
                        unreal.TextJustify.RIGHT})
    write_slot(P, hint_slot, {"size": unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL),
                              "padding": unreal.Margin(24, 0, 24, 0),
                              "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})
    back = UTIL.find_widget(wbp, "BackButton")
    if back.get_parent().get_name() != "Header":
        UTIL.attach_widget(wbp, "Header", "BackButton")
    write_slot(P, back.get_editor_property("slot"), {"vertical_alignment": unreal.VerticalAlignment.V_ALIGN_CENTER})

    _, grid_slot = add(wbp, P, unreal.UniformGridPanel, "CardGrid", "AbilityList", {"slot_padding": GRID_GAP})
    write_slot(P, grid_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_FILL})

    UTIL.commit_tree(wbp)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    page = unreal.get_default_object(wbp.generated_class())
    asset_guard.write(P, page, "card_class", card.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)


def run():
    text_table = build_style_table(TEXT_TABLE_PATH, ROLE.BODY)
    meta_table = build_style_table(META_TABLE_PATH, ROLE.LABEL)
    card = build_card(text_table, meta_table)
    build_page(card)
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "ability_page.txt")
    open(output, "w").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
