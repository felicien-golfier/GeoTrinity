"""
The Gems menu in the Rail look, opened from the character sheet's GEMS (design: the GeoTrinity Gems canvas).
- WBP_GemLoadout (UGeoGemLoadoutWidget): the build strip (BuildRow: the build tabs, their pencil, cross and dashed add
  tab drawn from the Menu_ icons, then the build in play and the build count on the right) over a divider; the socket
  board (Board, a 684 x 639 canvas the widget fills with the clusters, the class level block in its top-left corner);
  the stacks under their tier filters (WBP_TabUnderline, spread over a divider), scrolling in StackScroll; right of a
  divider, the selection (glyph, tier, name, effect, counts, EQUIP / FILL / UNEQUIP, note) or the hint, then the total
  bonus table (TotalsTable, a WBP_Table: STAT, BONUS, QTY).
- WBP_GemForge (UGeoGemForgeWidget): the tiers to select, BREAK DOWN FREE SELECTED and the rate table (RateTable, a
  WBP_Table: TIER, BREAK DOWN, CRAFT); the gem types
  of a tier, scrolling in GemScroll; the picked type broken down or crafted by a quantity typed in its field or set by
  -, + and MAX; dividers above the rate table, under the tier tabs (WBP_TabUnderline), above BREAK DOWN and CRAFT.
- WBP_Gems (UGeoGemsWidget): title, page tabs, the Loadout's class tabs (ClassTabBox) and the shards in a key cap over
  a PageSwitcher holding both pages; BACK is the page frame's, which menu_pages.py puts on it.
- WBP_TabUnderline: a WBP_ListRow-like row reading as a tab, its text over a bar lit on the tab picked.
- WBP_PauseMenu: GemsWidget beside CharacterWidget, collapsed.
Every other list, tab and row the widgets build at runtime is a WBP_ListRow (RowClass).

Never overwrites a hand edit: the widgets are built only when created, the pause menu only gains what it lacks, and
every value goes through asset_guard.write (kept values are listed in AI/Output/gems_page.txt).
Usage: run via MCP execute_script, after character_sheet.py (whose GEMS button opens it) and import_icons.py.
"""
import importlib
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import character_sheet
import rail_style

character_sheet = importlib.reload(character_sheet)
rail_style = importlib.reload(rail_style)
table = character_sheet.table
wings_hud = character_sheet.wings_hud
asset_guard = wings_hud.asset_guard
add, write_slot, text = wings_hud.add, wings_hud.write_slot, wings_hud.text
remove_unless_edited = wings_hud.remove_unless_edited
frame, meter = wings_hud.frame, wings_hud.meter
label, spacer, button = character_sheet.label, character_sheet.spacer, character_sheet.button

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
ROLE = unreal.GeoTextRole
MENU_DIR = character_sheet.MENU_DIR
LOADOUT_PATH = f"{MENU_DIR}/WBP_GemLoadout"
FORGE_PATH = f"{MENU_DIR}/WBP_GemForge"
GEMS_PATH = f"{MENU_DIR}/WBP_Gems"
ROW_CLASS = "/Game/HUD/WBP_ListRow.WBP_ListRow_C"
CENTER = unreal.VerticalAlignment.V_ALIGN_CENTER
FILL = unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)
# The forge's left column: the tier rows with their counts, and the break-down summary.
LEFT_WIDTH = 540.0
BREAK_SELECTED = "BREAK DOWN SELECTED"
FIELD_WIDTH = 72.0
UNDERLINE_TAB_PATH = "/Game/HUD/WBP_TabUnderline"
SHARD_ICON = "/Game/HUD/Icons/DA_Icon_Gems_Shard"
# The Loadout's socket board and selection column, and the total bonus table's columns, as the design sizes them.
BOARD_SIZE = (684.0, 639.0)
DETAIL_WIDTH = 305.0
TOTALS_COLUMNS = [table.column("STAT", alignment=table.FILL_ALIGN), table.column("BONUS", 72.0),
                  table.column("QTY", 44.0)]
# The Forge's rates, three even columns.
RATE_COLUMNS = [table.column("TIER", alignment=table.FILL_ALIGN), table.column("BREAK DOWN"), table.column("CRAFT")]
# The widgets the total bonus table was built from before WBP_Table.
OLD_TOTALS = ["TotalsColumns", "StatColumnText", "BonusColumnSize", "BonusColumnText", "QtyColumnSize", "QtyColumnText",
              "TotalsScroll", "TotalsBox"]


def sized(wbp, path, name, parent, width=None, height=None):
    """A SizeBox named name fixing width and/or height."""
    values = {}
    if width is not None:
        values.update(override_width_override=True, width_override=width)
    if height is not None:
        values.update(override_height_override=True, height_override=height)
    return add(wbp, path, unreal.SizeBox, name, parent, values)[1]


def padded(path, slot, padding, **extra):
    write_slot(path, slot, dict(padding=padding, **extra))


def break_summary(wbp, path):
    """The framed line under BREAK DOWN SELECTED, in the button's text size: the gems it destroys on the left, the
    shards it gives on the right, each in its colour. It replaces the small line a forge built before it had there."""
    if UTIL.find_widget(wbp, "BreakAllText") and not any(".BreakAllText" in line for line in asset_guard.hand_edits(path)):
        UTIL.remove_widget(wbp, "BreakAllText")
    if not UTIL.find_widget(wbp, "BreakSummary"):
        UTIL.construct_widget_in_tree(wbp, unreal.GeoFrame, "BreakSummary", True)
        after = UTIL.find_widget(wbp, "LeftBox").get_child_index(UTIL.find_widget(wbp, "BreakAllButton")) + 1
        UTIL.attach_widget(wbp, "LeftBox", "BreakSummary", after)
    summary = UTIL.find_widget(wbp, "BreakSummary")
    for key, value in dict(frame_style=frame("DA_Frame_Panel"), padding=unreal.Margin(18, 10, 18, 10)).items():
        asset_guard.write(path, summary, key, value)
    padded(path, summary.get_editor_property("slot"), unreal.Margin(0, 8, 0, 0))
    add(wbp, path, unreal.HorizontalBox, "BreakSummaryRow", "BreakSummary")
    _, count_slot = add(wbp, path, unreal.GeoText, "BreakCountText", "BreakSummaryRow", text(ROLE.BUTTON, "-0 GEMS"))
    write_slot(path, count_slot, {"size": FILL, "vertical_alignment": CENTER})
    _, shards_slot = add(wbp, path, unreal.GeoText, "BreakShardsText", "BreakSummaryRow", text(ROLE.BUTTON, "+0 SHARDS"))
    write_slot(path, shards_slot, {"vertical_alignment": CENTER})


def quantity_cell(wbp, path, row, name, caption, width):
    """A menu button showing caption, or with no caption the quantity field in its field frame, in a SizeBox width
    wide."""
    box_slot = sized(wbp, path, f"{name}Width", row, width=width)
    write_slot(path, box_slot, {"vertical_alignment": CENTER})
    if caption is None:
        if not UTIL.find_widget(wbp, f"{name}Frame"):
            UTIL.construct_widget_in_tree(wbp, unreal.GeoFrame, f"{name}Frame", True)
            if not UTIL.find_widget(wbp, name):
                UTIL.construct_widget_in_tree(wbp, unreal.GeoEditableTextBox, name, True)
            rail_style.fill_slot(UTIL.attach_widget(wbp, f"{name}Frame", name))
            UTIL.attach_widget(wbp, f"{name}Width", f"{name}Frame")
        for key, value in dict(frame_style=frame("DA_Frame_Field"), padding=rail_style.NO_PADDING,
                               activate_on_hover_and_focus=True).items():
            asset_guard.write(path, UTIL.find_widget(wbp, f"{name}Frame"), key, value)
        write_slot(path, UTIL.find_widget(wbp, f"{name}Frame").get_editor_property("slot"),
                   {"vertical_alignment": CENTER})
        for key, value in dict(text=unreal.Text("1"), justification=unreal.TextJustify.CENTER,
                               select_all_text_when_focused=True).items():
            asset_guard.write(path, UTIL.find_widget(wbp, name), key, value)
    else:
        button(wbp, path, name, f"{name}Width", caption)


def quantity_row(wbp, path, prefix):
    """-, the quantity field, +, MAX; commit_tree spaces them, and the row from the button under it, by the gap."""
    row = f"{prefix}QuantityRow"
    _, row_slot = add(wbp, path, unreal.HorizontalBox, row, "RightBox")
    padded(path, row_slot, unreal.Margin(0, 10, 0, 0))
    for name, caption, width in [(f"{prefix}LessButton", "-", 64.0), (f"{prefix}QuantityBox", None, FIELD_WIDTH),
                                 (f"{prefix}MoreButton", "+", 64.0), (f"{prefix}MaxButton", "MAX", 110.0)]:
        quantity_cell(wbp, path, row, name, caption, width)


def quantity_fields(wbp, path):
    """On a forge built before the quantities could be typed: each quantity text becomes the field in its place."""
    hand_edits = asset_guard.hand_edits(path)
    for prefix in ("Break", "Craft"):
        old, name, row = f"{prefix}QuantityText", f"{prefix}QuantityBox", f"{prefix}QuantityRow"
        if UTIL.find_widget(wbp, old) and not any(f".{old}" in line for line in hand_edits):
            index = UTIL.find_widget(wbp, row).get_child_index(UTIL.find_widget(wbp, f"{old}Width"))
            UTIL.remove_widget(wbp, old)
            UTIL.remove_widget(wbp, f"{old}Width")
            sized(wbp, path, f"{name}Width", row, width=FIELD_WIDTH)
            UTIL.attach_widget(wbp, row, f"{name}Width", index)
        quantity_cell(wbp, path, row, name, None, FIELD_WIDTH)
        padded(path, UTIL.find_widget(wbp, row).get_editor_property("slot"), unreal.Margin(0, 10, 0, 0))


def build_row(wbp, path):
    """The build strip under the class row: BuildTabBox, which the widget fills with the tabs, then the class shape,
    the build in play and the build count on the right."""
    if not UTIL.find_widget(wbp, "BuildRow"):
        UTIL.construct_widget_in_tree(wbp, unreal.HorizontalBox, "BuildRow", True)
        after = UTIL.find_widget(wbp, "LoadoutBody").get_child_index(UTIL.find_widget(wbp, "ClassRow")) + 1
        UTIL.attach_widget(wbp, "LoadoutBody", "BuildRow", after)
    padded(path, UTIL.find_widget(wbp, "BuildRow").get_editor_property("slot"), unreal.Margin(0, 0, 0, 14))
    _, tabs_slot = add(wbp, path, unreal.HorizontalBox, "BuildTabBox", "BuildRow")
    write_slot(path, tabs_slot, {"vertical_alignment": CENTER})
    spacer(wbp, path, "BuildSpacer", "BuildRow")
    _, shape_slot = add(wbp, path, unreal.GeoShape, "InPlayShape", "BuildRow", {"size": 10.0, "filled": True})
    padded(path, shape_slot, unreal.Margin(0, 0, 8, 0), vertical_alignment=CENTER)
    write_slot(path, label(wbp, path, "InPlayText", "BuildRow", "IN PLAY"), {"vertical_alignment": CENTER})
    count_slot = label(wbp, path, "BuildCountText", "BuildRow", "1 / 10", unreal.Margin(16, 0, 0, 0))
    write_slot(path, count_slot, {"vertical_alignment": CENTER})
    defaults = unreal.get_default_object(wbp.generated_class())
    for key, icon in [("rename_build_icon", "Menu_Rename"), ("remove_build_icon", "Menu_Remove"),
                      ("add_build_icon", "Menu_AddTab")]:
        asset_guard.write(path, defaults, key, unreal.load_asset(f"/Game/HUD/Icons/DA_Icon_{icon}"))


def move(wbp, name, parent, index=-1):
    """Puts the widget name under parent, at index, unless it is already there."""
    widget = UTIL.find_widget(wbp, name)
    current = widget.get_parent()
    if not current or current.get_name() != parent or (index >= 0 and current.get_child_index(widget) != index):
        UTIL.attach_widget(wbp, parent, name, index)
    return UTIL.find_widget(wbp, name).get_editor_property("slot")


def placed(wbp, path, widget_class, name, parent, index, values=None):
    """add, then at index in parent."""
    add(wbp, path, widget_class, name, parent, values)
    return move(wbp, name, parent, index)


def recreated(wbp, widget_class, name, values):
    """name built again, holding values, when the tree lost it. Its values are written directly: the ledger's record is
    its predecessor's, which a fresh widget never holds."""
    if not UTIL.find_widget(wbp, name):
        widget = UTIL.construct_widget_in_tree(wbp, widget_class, name, True)
        for key, value in values.items():
            widget.set_editor_property(key, value)


def divider(wbp, path, name, parent, index, vertical=False):
    """A one-pixel line in DA_Frame_Divider across parent, or down it when vertical, put at index when it is made."""
    size = f"{name}Size"
    values = (dict(override_width_override=True, width_override=1.0) if vertical
              else dict(override_height_override=True, height_override=1.0))
    if UTIL.find_widget(wbp, size):
        size_slot = add(wbp, path, unreal.SizeBox, size, parent, values)[1]
    else:
        size_slot = placed(wbp, path, unreal.SizeBox, size, parent, index, values)
    add(wbp, path, unreal.GeoFrame, name, size, {"frame_style": frame("DA_Frame_Divider"),
                                                  "padding": rail_style.NO_PADDING})
    rail_style.fill_slot(UTIL.find_widget(wbp, name).get_editor_property("slot"))
    return size_slot


def build_underline_tab():
    """WBP_TabUnderline: a list row reading as a tab of a strip, its text over a bar lit while it is the one picked
    (RowFrame in DA_Frame_Underline); its button and tints draw nothing."""
    path = UNDERLINE_TAB_PATH
    wbp = wings_hud.create_widget(path, "GeoListRowWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.VerticalBox, "TabRoot")
        add(wbp, path, unreal.GeoButton, "RowButton", "TabRoot")
        rail_style.clear_button_style(UTIL.find_widget(wbp, "RowButton"))
        add(wbp, path, unreal.VerticalBox, "TabBody", "RowButton")
        rail_style.fill_slot(UTIL.find_widget(wbp, "TabBody").get_editor_property("slot"))
        add(wbp, path, unreal.HorizontalBox, "ColumnsBox", "TabBody")
        sized(wbp, path, "UnderlineSize", "TabBody", height=2.0)
        add(wbp, path, unreal.GeoFrame, "RowFrame", "UnderlineSize",
            {"frame_style": frame("DA_Frame_Underline"), "padding": rail_style.NO_PADDING})
        wings_hud.finish(wbp)
        defaults = unreal.get_default_object(wbp.generated_class())
        clear = unreal.LinearColor(0, 0, 0, 0)
        for key, value in dict(normal_color=clear, alternate_color=clear, header_color=clear, selected_color=clear,
                               column_text_role=ROLE.LABEL, header_text_role=ROLE.LABEL,
                               column_padding=unreal.Margin(0, 0, 0, 8)).items():
            asset_guard.write(path, defaults, key, value)
        wings_hud.finish(wbp)
    return unreal.load_asset(path)


def loadout_layout(wbp, path, underline_tab):
    """The Loadout as the Gems design draws it: no class row (its tabs go to the Gems header, its level to the board's
    corner); the build strip over a divider; the board 684 x 639 with the level block in its top-left corner; the tier
    filters spread over a divider; a divider down the left of the selection; the total bonus under a divider, as a table
    whose header names its columns."""
    kept = remove_unless_edited(wbp, path, ["ClassTabBox", "ClassSpacer"])

    # The build strip, then its divider.
    padded(path, UTIL.find_widget(wbp, "BuildRow").get_editor_property("slot"), unreal.Margin(0, 0, 0, 12))
    move(wbp, "BuildRow", "LoadoutBody", 0)
    in_play_slot = UTIL.find_widget(wbp, "InPlayShape").get_editor_property("slot")
    # A square in the class colour; the builder first drew the class shape there at 10.
    for key, value, earlier in [("sides", 4, [6]), ("rotation", 45.0, [0.0]), ("size", 8.0, [10.0]),
                                ("filled", True, [])]:
        asset_guard.write(path, UTIL.find_widget(wbp, "InPlayShape"), key, value, earlier=earlier)
    padded(path, in_play_slot, unreal.Margin(0, 0, 8, 0), vertical_alignment=CENTER)
    shape_index = UTIL.find_widget(wbp, "BuildRow").get_child_index(UTIL.find_widget(wbp, "InPlayShape"))
    label_slot = placed(wbp, path, unreal.GeoText, "InPlayLabel", "BuildRow", shape_index + 1,
                        text(ROLE.LABEL, "IN PLAY"))
    padded(path, label_slot, unreal.Margin(0, 0, 8, 0), vertical_alignment=CENTER)
    padded(path, move(wbp, "InPlayText", "BuildRow", shape_index + 2), rail_style.NO_PADDING, vertical_alignment=CENTER)
    asset_guard.write(path, UTIL.find_widget(wbp, "InPlayText"), "role", ROLE.LABEL)
    padded(path, divider(wbp, path, "BuildDivider", "LoadoutBody", 1), unreal.Margin(0, 0, 0, 14))

    # The board, the level block in its corner.
    board_size = UTIL.find_widget(wbp, "BoardSize")
    asset_guard.write(path, board_size, "width_override", BOARD_SIZE[0], earlier=[640.0])
    asset_guard.write(path, board_size, "height_override", BOARD_SIZE[1], earlier=[534.0])
    if not UTIL.find_widget(wbp, "BoardOverlay"):
        UTIL.construct_widget_in_tree(wbp, unreal.Overlay, "BoardOverlay", True)
        recreated(wbp, unreal.CanvasPanel, "Board", {})
        # A SizeBox holds one child: the board leaves it for the overlay before the overlay takes its place.
        UTIL.attach_widget(wbp, "BoardOverlay", "Board")
        UTIL.attach_widget(wbp, "BoardSize", "BoardOverlay")
    recreated(wbp, unreal.GeoText, "LevelText", text(ROLE.HEADING, "1"))
    recreated(wbp, unreal.GeoMeter, "LevelPips", {"meter_style": meter("DA_Meter_LevelPips")})
    recreated(wbp, unreal.GeoText, "NextText", text(ROLE.LABEL, "NEXT"))
    rail_style.fill_slot(UTIL.find_widget(wbp, "Board").get_editor_property("slot"))
    level_size_slot = placed(wbp, path, unreal.SizeBox, "LevelBoxSize", "BoardOverlay", 1,
                             dict(override_width_override=True, width_override=160.0))
    write_slot(path, level_size_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_LEFT,
                                       "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_TOP})
    add(wbp, path, unreal.VerticalBox, "LevelBox", "LevelBoxSize")
    placed(wbp, path, unreal.GeoText, "LevelCaption", "LevelBox", 0, text(ROLE.LABEL, "CLASS LEVEL"))
    padded(path, placed(wbp, path, unreal.HorizontalBox, "LevelRow", "LevelBox", 1), unreal.Margin(0, 8, 0, 0))
    padded(path, move(wbp, "LevelText", "LevelRow", 0), rail_style.NO_PADDING,
           vertical_alignment=unreal.VerticalAlignment.V_ALIGN_BOTTOM)
    asset_guard.write(path, UTIL.find_widget(wbp, "LevelText"), "role", ROLE.HEADING, earlier=[ROLE.MONO])
    level_max_slot = placed(wbp, path, unreal.GeoText, "LevelMaxText", "LevelRow", 1, text(ROLE.LABEL, " / 20"))
    padded(path, level_max_slot, unreal.Margin(0, 0, 0, 3), vertical_alignment=unreal.VerticalAlignment.V_ALIGN_BOTTOM)
    pips_slot = placed(wbp, path, unreal.SizeBox, "LevelPipsSize", "LevelBox", 2,
                       dict(override_width_override=True, width_override=138.0,
                            override_height_override=True, height_override=14.0))
    padded(path, pips_slot, unreal.Margin(0, 8, 0, 0), horizontal_alignment=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    rail_style.fill_slot(move(wbp, "LevelPips", "LevelPipsSize"))
    padded(path, move(wbp, "NextText", "LevelBox", 3), unreal.Margin(0, 8, 0, 0))
    if UTIL.find_widget(wbp, "ClassRow") and not UTIL.find_widget(wbp, "ClassRow").get_children_count():
        UTIL.remove_widget(wbp, "ClassRow")

    # The stacks under their spread filters.
    padded(path, UTIL.find_widget(wbp, "InventoryColumn").get_editor_property("slot"), unreal.Margin(24, 0, 24, 0),
           size=FILL)
    padded(path, UTIL.find_widget(wbp, "TierFilterBox").get_editor_property("slot"), rail_style.NO_PADDING)
    padded(path, divider(wbp, path, "FilterDivider", "InventoryColumn", 1), unreal.Margin(0, 0, 0, 12))

    # The selection and the total bonus, right of a divider.
    divider(wbp, path, "DetailDivider", "Columns", 2, vertical=True)
    detail_size = UTIL.find_widget(wbp, "DetailColumnSize")
    asset_guard.write(path, detail_size, "width_override", DETAIL_WIDTH, earlier=[360.0])
    padded(path, detail_size.get_editor_property("slot"), unreal.Margin(24, 0, 0, 0))
    placed(wbp, path, unreal.SizeBox, "DetailSection", "DetailColumn", 0,
                          dict(override_min_desired_height=True, min_desired_height=236.0))
    add(wbp, path, unreal.VerticalBox, "DetailStack", "DetailSection")
    move(wbp, "DetailBox", "DetailStack", 0)
    move(wbp, "HintText", "DetailStack", 1)
    padded(path, divider(wbp, path, "TotalsDivider", "DetailColumn", 1), unreal.Margin(0, 16, 0, 14))
    placed(wbp, path, unreal.HorizontalBox, "TotalsHead", "DetailColumn", 2)
    padded(path, move(wbp, "TotalsTitleText", "TotalsHead", 0), rail_style.NO_PADDING)
    asset_guard.write(path, UTIL.find_widget(wbp, "TotalsTitleText"), "auto_wrap_text", False, earlier=[True])
    spacer(wbp, path, "TotalsHeadSpacer", "TotalsHead")
    write_slot(path, move(wbp, "TotalsHeadSpacer", "TotalsHead", 1), {"size": FILL})
    placed(wbp, path, unreal.GeoText, "TotalsFilledText", "TotalsHead", 2, text(ROLE.LABEL, "0 / 0 FILLED"))
    _, totals_kept = table.place(wbp, path, "TotalsTable", "DetailColumn", TOTALS_COLUMNS, OLD_TOTALS)
    padded(path, move(wbp, "TotalsTable", "DetailColumn", 3), unreal.Margin(0, 8, 0, 0), size=FILL)

    asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "tab_row_class",
                      underline_tab.generated_class())
    return kept + totals_kept


def forge_layout(wbp, path, underline_tab):
    """The Forge's dividers where its design draws them: above the rate table, under the tier tabs (WBP_TabUnderline),
    above the break-down and the craft blocks."""
    left = UTIL.find_widget(wbp, "LeftBox")
    padded(path, divider(wbp, path, "RateDivider", "LeftBox", left.get_child_index(UTIL.find_widget(wbp, "RateTable"))),
           unreal.Margin(0, 12, 0, 0))
    padded(path, UTIL.find_widget(wbp, "TierTabBox").get_editor_property("slot"), rail_style.NO_PADDING)
    middle = UTIL.find_widget(wbp, "MiddleBox")
    padded(path, divider(wbp, path, "TierDivider", "MiddleBox",
                         middle.get_child_index(UTIL.find_widget(wbp, "TierTabBox")) + 1), unreal.Margin(0, 0, 0, 12))
    for block in ("Break", "Craft"):
        right = UTIL.find_widget(wbp, "RightBox")
        index = right.get_child_index(UTIL.find_widget(wbp, f"{block}Label"))
        padded(path, divider(wbp, path, f"{block}Divider", "RightBox", index), unreal.Margin(0, 14, 0, 0))
        padded(path, UTIL.find_widget(wbp, f"{block}Label").get_editor_property("slot"), unreal.Margin(0, 14, 0, 0))
    asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "tab_row_class",
                      underline_tab.generated_class())


def gems_header(wbp, path):
    """The Gems header as the design draws it: title, page tabs, the Loadout's class tabs between two spacers, the
    shards in a key cap with the shard icon. The footer and its ESC BACK hint go: BACK is the page frame's."""
    kept = remove_unless_edited(wbp, path, ["Footer", "FooterSpacer", "HintText"])
    padded(path, UTIL.find_widget(wbp, "PageTabBox").get_editor_property("slot"), unreal.Margin(36, 0, 0, 0),
           vertical_alignment=CENTER)
    spacer(wbp, path, "HeaderLeftSpacer", "Header")
    write_slot(path, move(wbp, "HeaderLeftSpacer", "Header", 2), {"size": FILL})
    class_slot = placed(wbp, path, unreal.HorizontalBox, "ClassTabBox", "Header", 3)
    write_slot(path, class_slot, {"vertical_alignment": CENTER})
    move(wbp, "HeaderSpacer", "Header", 4)
    shards_slot = placed(wbp, path, unreal.GeoFrame, "ShardsFrame", "Header", 5,
                         {"frame_style": frame("DA_Frame_KeyCap"), "padding": unreal.Margin(16, 8, 16, 8)})
    write_slot(path, shards_slot, {"vertical_alignment": CENTER})
    add(wbp, path, unreal.HorizontalBox, "ShardsRow", "ShardsFrame")
    icon_slot = placed(wbp, path, unreal.GeoIconImage, "ShardIcon", "ShardsRow", 0,
                       {"icon": unreal.load_asset(SHARD_ICON), "size": 16.0})
    write_slot(path, icon_slot, {"vertical_alignment": CENTER})
    padded(path, move(wbp, "ShardsText", "ShardsRow", 1), unreal.Margin(12, 0, 0, 0), vertical_alignment=CENTER)
    padded(path, move(wbp, "ShardsLabel", "ShardsRow", 2), unreal.Margin(8, 0, 0, 0), vertical_alignment=CENTER)
    return kept


def build_loadout(underline_tab):
    path = LOADOUT_PATH
    wbp = wings_hud.create_widget(path, "GeoGemLoadoutWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.VerticalBox, "LoadoutBody")

        # Which class, and how far it is.
        _, class_row_slot = add(wbp, path, unreal.HorizontalBox, "ClassRow", "LoadoutBody")
        padded(path, class_row_slot, unreal.Margin(0, 0, 0, 14))
        add(wbp, path, unreal.HorizontalBox, "ClassTabBox", "ClassRow")
        spacer(wbp, path, "ClassSpacer", "ClassRow")
        _, level_slot = add(wbp, path, unreal.GeoText, "LevelText", "ClassRow", text(ROLE.MONO, "LEVEL 1 / 20"))
        write_slot(path, level_slot, {"vertical_alignment": CENTER})
        _, pips_slot = add(wbp, path, unreal.GeoMeter, "LevelPips", "ClassRow",
                           {"meter_style": meter("DA_Meter_LevelPips")})
        padded(path, pips_slot, unreal.Margin(18, 0, 18, 0), vertical_alignment=CENTER)
        next_slot = label(wbp, path, "NextText", "ClassRow", "NEXT")
        write_slot(path, next_slot, {"vertical_alignment": CENTER})

        _, columns_slot = add(wbp, path, unreal.HorizontalBox, "Columns", "LoadoutBody")
        write_slot(path, columns_slot, {"size": FILL})

        # The socket board.
        sized(wbp, path, "BoardSize", "Columns", width=640.0, height=534.0)
        add(wbp, path, unreal.CanvasPanel, "Board", "BoardSize")

        # The stacks.
        _, inventory_slot = add(wbp, path, unreal.VerticalBox, "InventoryColumn", "Columns")
        padded(path, inventory_slot, unreal.Margin(32, 0, 32, 0), size=FILL)
        _, filter_slot = add(wbp, path, unreal.HorizontalBox, "TierFilterBox", "InventoryColumn")
        padded(path, filter_slot, unreal.Margin(0, 0, 0, 12))
        add(wbp, path, unreal.UniformGridPanel, "StackGrid", "InventoryColumn",
            {"slot_padding": unreal.Margin(4, 4, 4, 4)})

        # The selection and the total bonus.
        sized(wbp, path, "DetailColumnSize", "Columns", width=360.0)
        add(wbp, path, unreal.VerticalBox, "DetailColumn", "DetailColumnSize")
        add(wbp, path, unreal.VerticalBox, "DetailBox", "DetailColumn")
        add(wbp, path, unreal.HorizontalBox, "DetailHead", "DetailBox")
        _, glyph_slot = add(wbp, path, unreal.GeoGemGlyph, "DetailGlyph", "DetailHead", {"size": 64.0})
        write_slot(path, glyph_slot, {"vertical_alignment": CENTER})
        _, title_slot = add(wbp, path, unreal.VerticalBox, "DetailTitle", "DetailHead")
        padded(path, title_slot, unreal.Margin(18, 0, 0, 0), vertical_alignment=CENTER)
        label(wbp, path, "DetailTierText", "DetailTitle", "TIER 1 · CHIP")
        add(wbp, path, unreal.GeoText, "DetailNameText", "DetailTitle", text(ROLE.HEADING, "GEM"))
        _, effect_slot = add(wbp, path, unreal.GeoText, "DetailEffectText", "DetailBox",
                             text(ROLE.BODY, "Effect", auto_wrap_text=True))
        padded(path, effect_slot, unreal.Margin(0, 14, 0, 0))
        _, counts_slot = add(wbp, path, unreal.GeoText, "DetailCountsText", "DetailBox",
                             text(ROLE.MONO, "OWNED 0 · SLOTTED 0 · FREE 0", auto_wrap_text=True))
        padded(path, counts_slot, unreal.Margin(0, 8, 0, 0))
        _, buttons_slot = add(wbp, path, unreal.HorizontalBox, "DetailButtons", "DetailBox")
        padded(path, buttons_slot, unreal.Margin(0, 16, 0, 0))
        for name, caption in [("EquipButton", "EQUIP"), ("FillButton", "FILL"), ("UnequipButton", "UNEQUIP")]:
            button(wbp, path, name, "DetailButtons", caption)
            write_slot(path, UTIL.find_widget(wbp, name).get_editor_property("slot"), {"size": FILL})
        _, note_slot = add(wbp, path, unreal.GeoText, "DetailNoteText", "DetailBox",
                           text(ROLE.LABEL, "", auto_wrap_text=True))
        padded(path, note_slot, unreal.Margin(0, 8, 0, 0))
        add(wbp, path, unreal.GeoText, "HintText", "DetailColumn", text(ROLE.BODY, "Pick a stack.", auto_wrap_text=True))
        label(wbp, path, "TotalsTitleText", "DetailColumn", "TOTAL BONUS", unreal.Margin(0, 22, 0, 10))
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "row_class",
                          unreal.load_class(None, ROW_CLASS))
        wings_hud.finish(wbp)
    wbp = unreal.load_asset(path)
    asset_guard.write(path, UTIL.find_widget(wbp, "TotalsTitleText"), "auto_wrap_text", True)
    build_row(wbp, path)
    wings_hud.scrolled(wbp, path, "StackGrid", "StackScroll")
    kept = loadout_layout(wbp, path, underline_tab)
    wings_hud.finish(wbp)
    return wbp, kept


def build_forge(underline_tab):
    path = FORGE_PATH
    wbp = wings_hud.create_widget(path, "GeoGemForgeWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.HorizontalBox, "ForgeBody")

        # The tiers to select, what breaking them down destroys and gives, and the button that does it.
        sized(wbp, path, "LeftSize", "ForgeBody", width=LEFT_WIDTH)
        add(wbp, path, unreal.VerticalBox, "LeftBox", "LeftSize")
        label(wbp, path, "BreakDownLabel", "LeftBox", "BREAK DOWN · SLOTTED GEMS ARE KEPT")
        _, tiers_slot = add(wbp, path, unreal.VerticalBox, "TierBreakBox", "LeftBox")
        padded(path, tiers_slot, unreal.Margin(0, 12, 0, 4))
        button(wbp, path, "BreakAllButton", "LeftBox", BREAK_SELECTED)
        spacer(wbp, path, "LeftSpacer", "LeftBox")
        table.place(wbp, path, "RateTable", "LeftBox", RATE_COLUMNS)

        # The gem types of a tier.
        _, middle_slot = add(wbp, path, unreal.VerticalBox, "MiddleBox", "ForgeBody")
        padded(path, middle_slot, unreal.Margin(32, 0, 32, 0), size=FILL)
        _, message_slot = add(wbp, path, unreal.GeoText, "MessageText", "MiddleBox", text(ROLE.MONO, ""))
        padded(path, message_slot, unreal.Margin(0, 0, 0, 8))
        _, tabs_slot = add(wbp, path, unreal.HorizontalBox, "TierTabBox", "MiddleBox")
        padded(path, tabs_slot, unreal.Margin(0, 0, 0, 12))
        add(wbp, path, unreal.VerticalBox, "GemListBox", "MiddleBox")

        # The picked type, by quantity.
        sized(wbp, path, "RightSize", "ForgeBody", width=380.0)
        add(wbp, path, unreal.VerticalBox, "RightBox", "RightSize")
        add(wbp, path, unreal.HorizontalBox, "PickedHead", "RightBox")
        _, glyph_slot = add(wbp, path, unreal.GeoGemGlyph, "PickedGlyph", "PickedHead", {"size": 72.0})
        write_slot(path, glyph_slot, {"vertical_alignment": CENTER})
        _, title_slot = add(wbp, path, unreal.VerticalBox, "PickedTitle", "PickedHead")
        padded(path, title_slot, unreal.Margin(18, 0, 0, 0), vertical_alignment=CENTER)
        label(wbp, path, "PickedTierText", "PickedTitle", "TIER 1 · CHIP")
        add(wbp, path, unreal.GeoText, "PickedNameText", "PickedTitle", text(ROLE.HEADING, "GEM"))
        _, effect_slot = add(wbp, path, unreal.GeoText, "PickedEffectText", "RightBox",
                             text(ROLE.BODY, "Effect", auto_wrap_text=True))
        padded(path, effect_slot, unreal.Margin(0, 14, 0, 0))
        _, counts_slot = add(wbp, path, unreal.GeoText, "PickedCountsText", "RightBox",
                             text(ROLE.MONO, "OWNED 0 · SLOTTED 0 · FREE 0", auto_wrap_text=True))
        padded(path, counts_slot, unreal.Margin(0, 8, 0, 0))
        label(wbp, path, "BreakLabel", "RightBox", "BREAK DOWN", unreal.Margin(0, 22, 0, 0))
        quantity_row(wbp, path, "Break")
        button(wbp, path, "BreakButton", "RightBox", "BREAK DOWN")
        label(wbp, path, "CraftLabel", "RightBox", "CRAFT", unreal.Margin(0, 22, 0, 0))
        quantity_row(wbp, path, "Craft")
        button(wbp, path, "CraftButton", "RightBox", "CRAFT")
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "row_class",
                          unreal.load_class(None, ROW_CLASS))
        wings_hud.finish(wbp)
    wbp = unreal.load_asset(path)
    # The tier rows' counts and the BREAK DOWN ALL FREE summary first ran out of a 430 wide column.
    asset_guard.write(path, UTIL.find_widget(wbp, "LeftSize"), "width_override", LEFT_WIDTH, earlier=(430.0, 500.0))
    break_summary(wbp, path)
    asset_guard.write(path, UTIL.find_widget(wbp, "BreakAllButton"), "label", unreal.Text(BREAK_SELECTED))
    quantity_fields(wbp, path)
    rate_slot, kept = table.place(wbp, path, "RateTable", "LeftBox", RATE_COLUMNS, ["RateBox"])
    padded(path, rate_slot, unreal.Margin(0, 12, 0, 0))
    wings_hud.scrolled(wbp, path, "GemListBox", "GemScroll")
    forge_layout(wbp, path, underline_tab)
    wings_hud.finish(wbp)
    return wbp, kept


def build_gems(loadout, forge):
    path = GEMS_PATH
    wbp = wings_hud.create_widget(path, "GeoGemsWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.GeoFrame, "GemsFrame")
        for key, value in dict(frame_style=frame("DA_Frame_Wing"), padding=unreal.Margin(40, 26, 40, 26)).items():
            asset_guard.write(path, UTIL.find_widget(wbp, "GemsFrame"), key, value)
        add(wbp, path, unreal.VerticalBox, "GemsBody", "GemsFrame")

        _, header_slot = add(wbp, path, unreal.HorizontalBox, "Header", "GemsBody")
        padded(path, header_slot, unreal.Margin(0, 0, 0, 16))
        _, title_slot = add(wbp, path, unreal.GeoText, "TitleText", "Header", text(ROLE.TITLE, "GEMS"))
        write_slot(path, title_slot, {"vertical_alignment": CENTER})
        _, tabs_slot = add(wbp, path, unreal.HorizontalBox, "PageTabBox", "Header")
        padded(path, tabs_slot, unreal.Margin(48, 0, 0, 0), vertical_alignment=CENTER)
        spacer(wbp, path, "HeaderSpacer", "Header")
        _, shards_slot = add(wbp, path, unreal.GeoText, "ShardsText", "Header", text(ROLE.MONO, "0"))
        write_slot(path, shards_slot, {"vertical_alignment": CENTER})
        shards_label = label(wbp, path, "ShardsLabel", "Header", "SHARDS", unreal.Margin(10, 0, 0, 0))
        write_slot(path, shards_label, {"vertical_alignment": CENTER})

        _, switcher_slot = add(wbp, path, unreal.WidgetSwitcher, "PageSwitcher", "GemsBody")
        write_slot(path, switcher_slot, {"size": FILL})
        add(wbp, path, loadout.generated_class(), "LoadoutPage", "PageSwitcher")
        add(wbp, path, forge.generated_class(), "ForgePage", "PageSwitcher")
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "row_class",
                          unreal.load_class(None, ROW_CLASS))
        wings_hud.finish(wbp)
    wbp = unreal.load_asset(path)
    kept = gems_header(wbp, path)
    wings_hud.finish(wbp)
    return wbp, kept


def add_to_pause_menu(gems):
    path = character_sheet.PAUSE_PATH
    wbp = unreal.load_asset(path)
    if not UTIL.find_widget(wbp, "GemsWidget"):
        character = UTIL.find_widget(wbp, "CharacterWidget")
        UTIL.construct_widget_in_tree(wbp, gems.generated_class(), "GemsWidget", True)
        UTIL.attach_widget(wbp, character.get_parent().get_name(), "GemsWidget")
    gems_slot = UTIL.find_widget(wbp, "GemsWidget").get_editor_property("slot")
    character_slot = UTIL.find_widget(wbp, "CharacterWidget").get_editor_property("slot")
    if isinstance(character_slot, unreal.CanvasPanelSlot):
        copied = ("layout_data", "z_order")
    else:
        copied = ("horizontal_alignment", "vertical_alignment", "padding")
    for prop in copied:
        asset_guard.write(path, gems_slot, prop, character_slot.get_editor_property(prop))
    asset_guard.write(path, UTIL.find_widget(wbp, "GemsWidget"), "visibility", unreal.SlateVisibility.COLLAPSED)
    wings_hud.finish(wbp)


def build_shard_icon():
    import import_icons
    import_icons = importlib.reload(import_icons)
    import_icons.asset_guard, import_icons.rail_style = asset_guard, rail_style
    import_icons.build_icon("Gems_Shard", os.path.join(import_icons.source_folder(), "Gems_Shard.svg"))


def run():
    build_shard_icon()
    underline_tab = build_underline_tab()
    loadout, loadout_kept = build_loadout(underline_tab)
    forge, forge_kept = build_forge(underline_tab)
    gems, gems_kept = build_gems(loadout, forge)
    add_to_pause_menu(gems)
    kept = asset_guard.report() + [f"not removed, edited by hand: {line}"
                                   for line in loadout_kept + forge_kept + gems_kept]
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "gems_page.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
