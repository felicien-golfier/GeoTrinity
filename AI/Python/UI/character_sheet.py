"""
The character sheet in the Rail look, opened from the pause menu.
- DA_Meter_LevelPips / DA_Meter_Xp: the level pips and the XP bar under the class name.
- WBP_AbilityRow (UGeoAbilityCardWidget): an ability in one compact row — icon tile, name, key cap, description.
- WBP_CharacterSheet (UGeoCharacterSheetWidget): header (class shape, name, role and player, level, pips, XP, the
  other classes), three columns (stats and this fight / abilities, scrolling in AbilityScroll / gems slotted, core
  effects, owned), footer (GEMS, ABILITY DETAILS).
- WBP_PauseMenu: CharacterButton under AbilitiesButton and CharacterWidget beside AbilitiesWidget.

Never overwrites a hand edit: the widgets are built only when created, the pause menu only gains what it lacks, and
every value goes through asset_guard.write (kept values are listed in AI/Output/character_sheet.txt).
Usage: run via MCP execute_script, after ui_theme.py and ability_page.py.
"""
import importlib
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import wings_hud

wings_hud = importlib.reload(wings_hud)
asset_guard = wings_hud.asset_guard
add, write_slot, text = wings_hud.add, wings_hud.write_slot, wings_hud.text
frame, meter = wings_hud.frame, wings_hud.meter
srgb = wings_hud.srgb

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
ROLE = unreal.GeoTextRole
MENU_DIR = "/Game/HUD/InGameMenu"
ROW_PATH = f"{MENU_DIR}/WBP_AbilityRow"
SHEET_PATH = f"{MENU_DIR}/WBP_CharacterSheet"
PAUSE_PATH = f"{MENU_DIR}/WBP_PauseMenu"
BUTTON_CLASS = "/Game/HUD/WBP_GeoButton.WBP_GeoButton_C"
CENTER = unreal.VerticalAlignment.V_ALIGN_CENTER
FILL = unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)
RIGHT = unreal.HorizontalAlignment.H_ALIGN_RIGHT

METER_STYLES = {
    # One pip per class level, lit in the class colour; the ticks are the gaps between pips.
    "DA_Meter_LevelPips": dict(
        shape=unreal.GeoMeterShape.BAR, size=unreal.Vector2D(276, 16), track_color=srgb("1C1730"),
        low_track_color=srgb("1C1730"), fill_color=srgb("FFFFFF"), glow_thickness=5.0, glow_opacity=.5,
        tick_parts=20, tick_color=srgb("080612"), tick_width=4.0),
    # XP towards the next level.
    "DA_Meter_Xp": dict(
        shape=unreal.GeoMeterShape.BAR, size=unreal.Vector2D(236, 6), track_color=srgb("1C1730"),
        low_track_color=srgb("1C1730"), fill_color=srgb("FFFFFF"), glow_thickness=6.0, glow_opacity=.6),
}


def build_meter_styles():
    for name, values in METER_STYLES.items():
        path = f"{wings_hud.STYLE_DIR}/{name}"
        style, created = wings_hud.ui_theme.create_or_load(path, unreal.GeoMeterStyle)
        if created:
            asset_guard.created(path)
        for key, value in values.items():
            asset_guard.write(path, style, key, value)
        unreal.EditorAssetLibrary.save_loaded_asset(style)


def build_ability_row():
    path = ROW_PATH
    wbp = wings_hud.create_widget(path, "GeoAbilityCardWidget")
    if wbp:
        text_table = unreal.load_asset(f"{wings_hud.STYLE_DIR}/DT_AbilityText")
        meta_table = unreal.load_asset(f"{wings_hud.STYLE_DIR}/DT_AbilityMeta")
        UTIL.set_root_panel(wbp, unreal.HorizontalBox, "Row")
        _, tile_slot = add(wbp, path, unreal.GeoFrame, "IconFrame", "Row",
                           {"frame_style": frame("DA_Frame_IconTile"), "padding": unreal.Margin(9, 9, 9, 9)})
        write_slot(path, tile_slot, {"vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoIconImage, "Icon", "IconFrame", {"size": 29.0})
        _, column_slot = add(wbp, path, unreal.VerticalBox, "TextColumn", "Row")
        write_slot(path, column_slot,
                   {"size": FILL, "padding": unreal.Margin(16, 0, 0, 0), "vertical_alignment": CENTER})
        add(wbp, path, unreal.HorizontalBox, "TitleRow", "TextColumn")
        _, name_slot = add(wbp, path, unreal.GeoText, "NameText", "TitleRow", text(ROLE.BUTTON, "ABILITY"))
        write_slot(path, name_slot, {"vertical_alignment": CENTER})
        _, key_slot = add(wbp, path, unreal.GeoFrame, "KeyFrame", "TitleRow",
                          {"frame_style": frame("DA_Frame_KeyCap"), "padding": unreal.Margin(8, 1, 8, 1)})
        write_slot(path, key_slot, {"padding": unreal.Margin(10, 0, 0, 0), "vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoText, "KeyText", "KeyFrame", text(ROLE.LABEL, "KEY"))
        _, timing_slot = add(wbp, path, unreal.RichTextBlock, "TimingText", "TitleRow",
                             {"text_style_set": meta_table, "visibility": unreal.SlateVisibility.COLLAPSED})
        write_slot(path, timing_slot, {"vertical_alignment": CENTER})
        _, description_slot = add(wbp, path, unreal.RichTextBlock, "DescriptionText", "TextColumn",
                                  {"text_style_set": text_table, "auto_wrap_text": True,
                                   "text": unreal.Text("Description.")})
        write_slot(path, description_slot, {"padding": unreal.Margin(0, 4, 0, 0)})
        add(wbp, path, unreal.VerticalBox, "BuffBox", "TextColumn")
        wings_hud.finish(wbp)
    return unreal.load_asset(path)


def label(wbp, path, name, parent, value, padding=None):
    _, slot = add(wbp, path, unreal.GeoText, name, parent, text(ROLE.LABEL, value))
    if padding:
        write_slot(path, slot, {"padding": padding})
    return slot


def spacer(wbp, path, name, parent):
    _, slot = add(wbp, path, unreal.Spacer, name, parent)
    write_slot(path, slot, {"size": FILL})


def column(wbp, path, name, parent, width):
    """A fixed-width SizeBox holding a VerticalBox named name."""
    box = f"{name}Width"
    add(wbp, path, unreal.SizeBox, box, parent, {"override_width_override": True, "width_override": width})
    add(wbp, path, unreal.VerticalBox, name, box)


def value_row(wbp, path, parent, label_text, value_name):
    row = f"{value_name}Row"
    _, row_slot = add(wbp, path, unreal.HorizontalBox, row, parent)
    write_slot(path, row_slot, {"padding": unreal.Margin(0, 8, 0, 0)})
    _, label_slot = add(wbp, path, unreal.GeoText, f"{value_name}Label", row, text(ROLE.BODY, label_text))
    write_slot(path, label_slot, {"size": FILL})
    add(wbp, path, unreal.GeoText, value_name, row, text(ROLE.MONO, "0"))


def button(wbp, path, name, parent, caption):
    """A menu button; commit_tree spaces neighbouring buttons by the theme's gap."""
    _, slot = add(wbp, path, unreal.load_class(None, BUTTON_CLASS), name, parent, {"label": unreal.Text(caption)})
    write_slot(path, slot, {"vertical_alignment": CENTER})


def build_sheet(row):
    path = SHEET_PATH
    wbp = wings_hud.create_widget(path, "GeoCharacterSheetWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.GeoFrame, "SheetFrame")
        for key, value in dict(frame_style=frame("DA_Frame_Wing"), padding=unreal.Margin(40, 28, 40, 28)).items():
            asset_guard.write(path, UTIL.find_widget(wbp, "SheetFrame"), key, value)
        add(wbp, path, unreal.VerticalBox, "SheetBody", "SheetFrame")

        # Header: who and how far.
        _, header_slot = add(wbp, path, unreal.HorizontalBox, "Header", "SheetBody")
        write_slot(path, header_slot, {"padding": unreal.Margin(0, 0, 0, 22)})
        _, shape_slot = add(wbp, path, unreal.GeoShape, "ClassShape", "Header", {"size": 64.0, "filled": True})
        write_slot(path, shape_slot, {"vertical_alignment": CENTER})
        _, title_slot = add(wbp, path, unreal.VerticalBox, "TitleColumn", "Header")
        write_slot(path, title_slot, {"padding": unreal.Margin(28, 0, 0, 0), "vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoText, "ClassNameText", "TitleColumn", text(ROLE.HEADING, "CLASS"))
        label(wbp, path, "SubtitleText", "TitleColumn", "ROLE  ·  PLAYER", unreal.Margin(0, 4, 0, 0))
        _, level_slot = add(wbp, path, unreal.VerticalBox, "LevelColumn", "Header")
        write_slot(path, level_slot, {"padding": unreal.Margin(36, 0, 0, 0), "vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoText, "LevelText", "LevelColumn", text(ROLE.MONO, "LEVEL 1  / 20"))
        _, pips_slot = add(wbp, path, unreal.GeoMeter, "LevelPips", "LevelColumn",
                           {"meter_style": meter("DA_Meter_LevelPips")})
        write_slot(path, pips_slot, {"padding": unreal.Margin(0, 10, 0, 0)})
        _, xp_row_slot = add(wbp, path, unreal.HorizontalBox, "XpRow", "LevelColumn")
        write_slot(path, xp_row_slot, {"padding": unreal.Margin(0, 10, 0, 0)})
        _, xp_slot = add(wbp, path, unreal.GeoMeter, "XpMeter", "XpRow", {"meter_style": meter("DA_Meter_Xp")})
        write_slot(path, xp_slot, {"vertical_alignment": CENTER})
        xp_text_slot = label(wbp, path, "XpText", "XpRow", "0 / 1000 XP", unreal.Margin(12, 0, 0, 0))
        write_slot(path, xp_text_slot, {"vertical_alignment": CENTER})
        label(wbp, path, "NextLevelText", "LevelColumn", "NEXT", unreal.Margin(0, 6, 0, 0))
        spacer(wbp, path, "HeaderSpacer", "Header")
        _, others_slot = add(wbp, path, unreal.VerticalBox, "OthersColumn", "Header")
        write_slot(path, others_slot, {"vertical_alignment": CENTER})
        others_label = label(wbp, path, "OthersLabel", "OthersColumn", "OTHER CLASSES")
        write_slot(path, others_label, {"horizontal_alignment": RIGHT})
        _, others_box_slot = add(wbp, path, unreal.HorizontalBox, "OtherClassesBox", "OthersColumn")
        write_slot(path, others_box_slot, {"padding": unreal.Margin(0, 10, 0, 0), "horizontal_alignment": RIGHT})

        # Three columns: stats, abilities, gems.
        _, columns_slot = add(wbp, path, unreal.HorizontalBox, "Columns", "SheetBody")
        write_slot(path, columns_slot, {"size": FILL})
        column(wbp, path, "StatsColumn", "Columns", 400.0)
        label(wbp, path, "StatsLabel", "StatsColumn", "STATS")
        _, stat_box_slot = add(wbp, path, unreal.VerticalBox, "StatBox", "StatsColumn")
        write_slot(path, stat_box_slot, {"padding": unreal.Margin(0, 12, 0, 0)})
        spacer(wbp, path, "StatsSpacer", "StatsColumn")
        label(wbp, path, "FightLabel", "StatsColumn", "THIS FIGHT", unreal.Margin(0, 14, 0, 0))
        value_row(wbp, path, "StatsColumn", "Damage / s", "FightDpsText")
        value_row(wbp, path, "StatsColumn", "Healing / s", "FightHpsText")
        value_row(wbp, path, "StatsColumn", "Damage taken", "FightTakenText")

        _, ability_column_slot = add(wbp, path, unreal.VerticalBox, "AbilityColumn", "Columns")
        write_slot(path, ability_column_slot, {"size": FILL, "padding": unreal.Margin(40, 0, 40, 0)})
        label(wbp, path, "AbilitiesLabel", "AbilityColumn", "ABILITIES")
        _, ability_box_slot = add(wbp, path, unreal.VerticalBox, "AbilityBox", "AbilityColumn")
        write_slot(path, ability_box_slot, {"padding": unreal.Margin(0, 18, 0, 0)})

        column(wbp, path, "GemsColumn", "Columns", 380.0)
        add(wbp, path, unreal.HorizontalBox, "SlottedRow", "GemsColumn")
        slotted_label = label(wbp, path, "SlottedLabel", "SlottedRow", "GEMS SLOTTED")
        write_slot(path, slotted_label, {"size": FILL})
        add(wbp, path, unreal.GeoText, "SlottedText", "SlottedRow", text(ROLE.MONO, "0 / 0"))
        _, gem_box_slot = add(wbp, path, unreal.WrapBox, "GemBox", "GemsColumn")
        write_slot(path, gem_box_slot, {"padding": unreal.Margin(0, 14, 0, 0)})
        label(wbp, path, "CoreLabel", "GemsColumn", "CORE EFFECTS", unreal.Margin(0, 14, 0, 0))
        _, core_slot = add(wbp, path, unreal.VerticalBox, "CoreEffectBox", "GemsColumn")
        write_slot(path, core_slot, {"padding": unreal.Margin(0, 10, 0, 0)})
        spacer(wbp, path, "GemsSpacer", "GemsColumn")
        _, owned_row_slot = add(wbp, path, unreal.HorizontalBox, "OwnedRow", "GemsColumn")
        write_slot(path, owned_row_slot, {"padding": unreal.Margin(0, 14, 0, 0)})
        owned_label = label(wbp, path, "OwnedLabel", "OwnedRow", "OWNED  ·  SHARDS")
        write_slot(path, owned_label, {"size": FILL})
        add(wbp, path, unreal.GeoText, "ShardsText", "OwnedRow", text(ROLE.MONO, "0"))
        label(wbp, path, "OwnedText", "GemsColumn", "CHIPS 0", unreal.Margin(0, 12, 0, 0))

        # Footer: where to go next.
        _, footer_slot = add(wbp, path, unreal.HorizontalBox, "Footer", "SheetBody")
        write_slot(path, footer_slot, {"padding": unreal.Margin(0, 16, 0, 0)})
        button(wbp, path, "GemLoadoutButton", "Footer", "GEMS")
        button(wbp, path, "AbilityDetailsButton", "Footer", "ABILITY DETAILS")
        spacer(wbp, path, "FooterSpacer", "Footer")
        hint_slot = label(wbp, path, "HintText", "Footer", "ESC  BACK")
        write_slot(path, hint_slot, {"vertical_alignment": CENTER})
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "ability_row_class",
                          row.generated_class())
        wings_hud.finish(wbp)
    wbp = unreal.load_asset(path)
    asset_guard.write(path, UTIL.find_widget(wbp, "GemLoadoutButton"), "label", unreal.Text("GEMS"))
    asset_guard.write(path, UTIL.find_widget(wbp, "OwnedText"), "auto_wrap_text", True)
    wings_hud.scrolled(wbp, path, "AbilityBox", "AbilityScroll")
    wings_hud.finish(wbp)
    return wbp


def add_to_pause_menu(sheet):
    path = PAUSE_PATH
    wbp = unreal.load_asset(path)
    if not UTIL.find_widget(wbp, "CharacterButton"):
        abilities_button = UTIL.find_widget(wbp, "AbilitiesButton")
        parent = abilities_button.get_parent()
        UTIL.construct_widget_in_tree(wbp, unreal.load_class(None, BUTTON_CLASS), "CharacterButton", True)
        UTIL.attach_widget(wbp, parent.get_name(), "CharacterButton", parent.get_child_index(abilities_button) + 1)
    asset_guard.write(path, UTIL.find_widget(wbp, "CharacterButton"), "label", unreal.Text("CHARACTER"))

    if not UTIL.find_widget(wbp, "CharacterWidget"):
        abilities = UTIL.find_widget(wbp, "AbilitiesWidget")
        UTIL.construct_widget_in_tree(wbp, sheet.generated_class(), "CharacterWidget", True)
        UTIL.attach_widget(wbp, abilities.get_parent().get_name(), "CharacterWidget")
    # Same place as the abilities page: its anchors on a canvas, its alignment and padding anywhere else.
    character_slot = UTIL.find_widget(wbp, "CharacterWidget").get_editor_property("slot")
    abilities_slot = UTIL.find_widget(wbp, "AbilitiesWidget").get_editor_property("slot")
    if isinstance(abilities_slot, unreal.CanvasPanelSlot):
        copied = ("layout_data", "z_order")
    else:
        copied = ("horizontal_alignment", "vertical_alignment", "padding")
    for prop in copied:
        asset_guard.write(path, character_slot, prop, abilities_slot.get_editor_property(prop))
    asset_guard.write(path, UTIL.find_widget(wbp, "CharacterWidget"), "visibility", unreal.SlateVisibility.COLLAPSED)
    wings_hud.finish(wbp)


def run():
    build_meter_styles()
    sheet = build_sheet(build_ability_row())
    add_to_pause_menu(sheet)
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "character_sheet.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
