"""
The character sheet in the Rail look, opened from the pause menu.
- DA_Meter_LevelPips / DA_Meter_Xp: the level pips and the XP bar under the class name.
- WBP_AbilityRow (UGeoAbilityCardWidget): an ability in one compact row, a button in the menu buttons' frame (CardFrame)
  — icon tile, name, key cap, its description cut to DESCRIPTION_MAX_HEIGHT (two lines) with an ellipsis, then the
  CLICK TO SEE DETAIL hint; no Reload buff lines.
- WBP_CharacterSheet (UGeoCharacterSheetWidget): header (class shape, name, role and player, level, pips, XP, the
  other classes), three columns (the stats in StatTable, a WBP_Table: STAT, VALUE, GEMS, BUFFS, filling the column /
  abilities, scrolling in AbilityScroll / this fight in FightTable: NOW, AVG, PEAK, TOTAL, then gems slotted, core
  effects, owned), footer (GEMS under the gems); over it all, in SheetOverlay, the ability drawer (DetailWidget,
  WBP_AbilityDetail from ability_detail.py) and its DetailScrim, both collapsed.
- IA_ShowCharacterSheet, rebindable as ShowCharacterSheet, on Tab and the gamepad's View button: BP_GeoPlayerController
  shows its own WBP_CharacterSheet while it is held, without BACK, the cross or GEMS.
- WBP_PauseMenu: CharacterButton under ResumeButton and CharacterWidget beside SettingsWidget; the abilities page
  (AbilitiesButton, AbilitiesWidget) removed.

Never overwrites a hand edit: the widgets are built only when created, the pause menu only gains what it lacks, and
every value goes through asset_guard.write (kept values are listed in AI/Output/character_sheet.txt).
Usage: run via MCP execute_script, after ui_theme.py, ability_page.py and ability_detail.py; builds WBP_Table
(table.py) first.
"""
import importlib
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import table

table = importlib.reload(table)
wings_hud = table.wings_hud
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
DETAIL_PATH = f"{MENU_DIR}/WBP_AbilityDetail"
# Two whole lines of the description's 18-point font: a line partly past it is dropped whole, ellipsis on the one above.
DESCRIPTION_MAX_HEIGHT = 72.0
EARLIER_DESCRIPTION_MAX_HEIGHT = [64.0]
# The row is a button: the menu buttons' frame round it, its padding the room inside the line.
ROW_FRAME_PADDING = unreal.Margin(14, 12, 18, 12)
FILL = unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)
RIGHT = unreal.HorizontalAlignment.H_ALIGN_RIGHT
STATS_WIDTH = 400.0
SHEET_ACTION = "/Game/Input/InputActions/IA_ShowCharacterSheet"
SHEET_KEYS = ("Tab", "Gamepad_Special_Left")
# The stats table: the stat, its live value, what the gems add, what the buffs add; this fight's figures.
STAT_COLUMNS = [table.column("STAT", alignment=table.FILL_ALIGN), table.column("VALUE", 72.0),
                table.column("GEMS", 72.0), table.column("BUFFS", 72.0)]
FIGHT_COLUMNS = [table.column("THIS FIGHT", alignment=table.FILL_ALIGN), table.column("NOW", 64.0),
                 table.column("AVG", 64.0), table.column("PEAK", 64.0), table.column("TOTAL", 64.0)]
# The figures this fight showed one per line before FightTable.
OLD_FIGHT_ROWS = [f"{name}{part}" for name in ("FightDpsText", "FightHpsText", "FightTakenText")
                  for part in ("Row", "Label", "")]
GEMS_WIDTH = 380.0

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


def button(wbp, path, name, parent, caption):
    """A menu button; commit_tree spaces neighbouring buttons by the theme's gap."""
    _, slot = add(wbp, path, unreal.load_class(None, BUTTON_CLASS), name, parent, {"label": unreal.Text(caption)})
    write_slot(path, slot, {"vertical_alignment": CENTER})


def build_footer(wbp, path):
    """GEMS under the gems column, nothing under the stats or the abilities."""
    _, footer_slot = add(wbp, path, unreal.HorizontalBox, "Footer", "SheetBody")
    write_slot(path, footer_slot, {"padding": unreal.Margin(0, 16, 0, 0)})
    add(wbp, path, unreal.SizeBox, "BackCellWidth", "Footer", {"override_width_override": True,
                                                              "width_override": STATS_WIDTH})
    spacer(wbp, path, "AbilityCell", "Footer")
    column_cell(wbp, path, "GemsCell", "GemLoadoutButton", "GEMS", GEMS_WIDTH)


def column_cell(wbp, path, name, button_name, caption, width):
    """A footer cell as wide as the column above it, holding its button at the left edge."""
    add(wbp, path, unreal.SizeBox, f"{name}Width", "Footer", {"override_width_override": True, "width_override": width})
    add(wbp, path, unreal.HorizontalBox, name, f"{name}Width")
    button(wbp, path, button_name, name, caption)


def migrate_footer(wbp, path):
    """Rebuilds the footer of a sheet built before its buttons sat under their columns, and drops ABILITY DETAILS: an
    ability opens its own details now. Returns the hand edits that kept a widget from going."""
    if not UTIL.find_widget(wbp, "GemsCell"):
        for name in ("HintText", "FooterSpacer", "GemLoadoutButton", "AbilityDetailsButton", "Footer"):
            UTIL.remove_widget(wbp, name)
        build_footer(wbp, path)
    kept = []
    if isinstance(UTIL.find_widget(wbp, "AbilityCell"), unreal.HorizontalBox):
        kept = wings_hud.remove_unless_edited(wbp, path, ["AbilityCell", "AbilityDetailsButton"])
        if not kept:
            spacer(wbp, path, "AbilityCell", "Footer")
            UTIL.attach_widget(wbp, "Footer", "AbilityCell", 1)
            write_slot(path, UTIL.find_widget(wbp, "AbilityCell").get_editor_property("slot"), {"size": FILL})
    wings_hud.finish(wbp)
    return kept


def row_summary(row):
    """The row is a button like the menu's (CardFrame in DA_Frame_Button round it, lit on hover and focus, and while its
    details are open), showing the start of the description, two lines cut with an ellipsis, and the hint that a click
    opens the rest; the Reload's buff lines are the drawer's alone."""
    path = ROW_PATH
    if not UTIL.find_widget(row, "CardFrame"):
        UTIL.construct_widget_in_tree(row, unreal.GeoFrame, "CardFrame", True)
        UTIL.attach_widget(row, "CardFrame", "Row")
        UTIL.set_root_widget(row, "CardFrame")
    for key, value in dict(frame_style=frame("DA_Frame_Button"), activate_on_hover_and_focus=True,
                           padding=ROW_FRAME_PADDING).items():
        asset_guard.write(path, UTIL.find_widget(row, "CardFrame"), key, value)
    if not UTIL.find_widget(row, "DescriptionClip"):
        description = UTIL.find_widget(row, "DescriptionText")
        column = description.get_parent()
        UTIL.construct_widget_in_tree(row, unreal.SizeBox, "DescriptionClip", True)
        UTIL.attach_widget(row, column.get_name(), "DescriptionClip", column.get_child_index(description))
        UTIL.attach_widget(row, "DescriptionClip", "DescriptionText")
    clip = UTIL.find_widget(row, "DescriptionClip")
    for key, value in dict(override_max_desired_height=True,
                           clipping=unreal.WidgetClipping.CLIP_TO_BOUNDS).items():
        asset_guard.write(path, clip, key, value)
    asset_guard.write(path, clip, "max_desired_height", DESCRIPTION_MAX_HEIGHT, earlier=EARLIER_DESCRIPTION_MAX_HEIGHT)
    write_slot(path, clip.get_editor_property("slot"), {"padding": unreal.Margin(0, 4, 0, 0)})
    asset_guard.write(path, UTIL.find_widget(row, "DescriptionText"), "text_overflow_policy",
                      unreal.TextOverflowPolicy.MULTILINE_ELLIPSIS)
    asset_guard.write(path, UTIL.find_widget(row, "BuffBox"), "visibility", unreal.SlateVisibility.COLLAPSED)
    label(row, path, "DetailHintText", "TextColumn", "CLICK TO SEE DETAIL", unreal.Margin(0, 6, 0, 0))
    wings_hud.finish(row)


def sheet_drawer(wbp, path, detail):
    """The ability drawer over the whole sheet: SheetOverlay takes the page frame's PageSlot, holding SheetBody, then
    DetailScrim (filling) and DetailWidget (right edge, full height), both collapsed until an ability is picked."""
    if not UTIL.find_widget(wbp, "SheetOverlay"):
        UTIL.construct_widget_in_tree(wbp, unreal.Overlay, "SheetOverlay", True)
        UTIL.set_named_slot_content(wbp, "PageFrame", "PageSlot", "SheetOverlay")
        UTIL.attach_widget(wbp, "SheetOverlay", "SheetBody")
    fill = {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_FILL,
            "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_FILL}
    write_slot(path, UTIL.find_widget(wbp, "SheetBody").get_editor_property("slot"), fill)
    _, scrim_slot = add(wbp, path, unreal.GeoFrame, "DetailScrim", "SheetOverlay",
                        {"frame_style": frame("DA_Frame_Scrim"), "visibility": unreal.SlateVisibility.COLLAPSED})
    write_slot(path, scrim_slot, fill)
    _, detail_slot = add(wbp, path, detail.generated_class(), "DetailWidget", "SheetOverlay",
                         {"visibility": unreal.SlateVisibility.COLLAPSED})
    write_slot(path, detail_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_RIGHT,
                                   "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_FILL})


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
        column(wbp, path, "StatsColumn", "Columns", STATS_WIDTH)
        label(wbp, path, "StatsLabel", "StatsColumn", "STATS")
        table.place(wbp, path, "StatTable", "StatsColumn", STAT_COLUMNS)
        table.place(wbp, path, "FightTable", "StatsColumn", FIGHT_COLUMNS)

        _, ability_column_slot = add(wbp, path, unreal.VerticalBox, "AbilityColumn", "Columns")
        write_slot(path, ability_column_slot, {"size": FILL, "padding": unreal.Margin(40, 0, 40, 0)})
        label(wbp, path, "AbilitiesLabel", "AbilityColumn", "ABILITIES")
        _, ability_box_slot = add(wbp, path, unreal.VerticalBox, "AbilityBox", "AbilityColumn")
        write_slot(path, ability_box_slot, {"padding": unreal.Margin(0, 18, 0, 0)})

        column(wbp, path, "GemsColumn", "Columns", GEMS_WIDTH)
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

        build_footer(wbp, path)
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "ability_row_class",
                          row.generated_class())
        wings_hud.finish(wbp)
    wbp = unreal.load_asset(path)
    kept = migrate_footer(wbp, path)
    stat_slot, stat_kept = table.place(wbp, path, "StatTable", "StatsColumn", STAT_COLUMNS, ["StatBox"])
    write_slot(path, stat_slot, {"padding": unreal.Margin(0, 12, 0, 0), "size": FILL})
    # The stats table fills the column now, its own header naming this fight's table under it.
    kept += stat_kept + wings_hud.remove_unless_edited(wbp, path, ["StatsSpacer", "FightLabel"])
    _, fight_kept = table.place(wbp, path, "FightTable", "StatsColumn", FIGHT_COLUMNS, OLD_FIGHT_ROWS)
    kept += fight_kept
    # This fight heads the gems column, leaving the stats column to the stats.
    fight = UTIL.find_widget(wbp, "FightTable")
    if fight.get_parent().get_name() != "GemsColumn":
        UTIL.attach_widget(wbp, "GemsColumn", "FightTable", 0)
    write_slot(path, UTIL.find_widget(wbp, "FightTable").get_editor_property("slot"),
               {"padding": unreal.Margin(0, 0, 0, 22)})
    asset_guard.write(path, UTIL.find_widget(wbp, "GemLoadoutButton"), "label", unreal.Text("GEMS"))
    asset_guard.write(path, UTIL.find_widget(wbp, "OwnedText"), "auto_wrap_text", True)
    wings_hud.scrolled(wbp, path, "AbilityBox", "AbilityScroll")
    if UTIL.find_widget(wbp, "PageFrame"):
        sheet_drawer(wbp, path, unreal.load_asset(DETAIL_PATH))
    wings_hud.finish(wbp)
    return wbp, kept


def add_to_pause_menu(sheet):
    path = PAUSE_PATH
    wbp = unreal.load_asset(path)
    if not UTIL.find_widget(wbp, "CharacterButton"):
        resume_button = UTIL.find_widget(wbp, "ResumeButton")
        parent = resume_button.get_parent()
        UTIL.construct_widget_in_tree(wbp, unreal.load_class(None, BUTTON_CLASS), "CharacterButton", True)
        UTIL.attach_widget(wbp, parent.get_name(), "CharacterButton", parent.get_child_index(resume_button) + 1)
    # An ability's details open on the sheet itself: the abilities page is gone.
    kept = wings_hud.remove_unless_edited(wbp, path, ["AbilitiesButton", "AbilitiesWidget"])
    asset_guard.write(path, UTIL.find_widget(wbp, "CharacterButton"), "label", unreal.Text("CHARACTER"))

    if not UTIL.find_widget(wbp, "CharacterWidget"):
        settings = UTIL.find_widget(wbp, "SettingsWidget")
        UTIL.construct_widget_in_tree(wbp, sheet.generated_class(), "CharacterWidget", True)
        UTIL.attach_widget(wbp, settings.get_parent().get_name(), "CharacterWidget")
    asset_guard.write(path, UTIL.find_widget(wbp, "CharacterWidget"), "visibility", unreal.SlateVisibility.COLLAPSED)
    wings_hud.finish(wbp)
    return kept


def run():
    table.build_table()
    build_meter_styles()
    row = build_ability_row()
    row_summary(row)
    sheet, sheet_kept = build_sheet(row)
    pause_kept = add_to_pause_menu(sheet)
    wings_hud.add_input(SHEET_ACTION, "ShowCharacterSheet", "Character sheet", SHEET_KEYS,
                        {"show_character_sheet_action": "action", "character_sheet_widget_class": sheet.generated_class()})
    kept = asset_guard.report() + [f"not removed, edited by hand: {line}" for line in sheet_kept + pause_kept]
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "character_sheet.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
