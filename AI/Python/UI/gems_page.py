"""
The Gems menu in the Rail look, opened from the character sheet's GEMS (design: the GeoTrinity Gems canvas).
- WBP_GemLoadout (UGeoGemLoadoutWidget): class tabs and level row; the socket board (Board, a 640 x 534 canvas the
  widget fills with the clusters); the stacks under their tier filter, scrolling in StackScroll; the selection (glyph,
  tier, name, effect, counts, EQUIP / FILL / UNEQUIP, note) or the hint; the total bonus, scrolling in TotalsScroll.
- WBP_GemForge (UGeoGemForgeWidget): the tiers to select, BREAK DOWN FREE SELECTED and the rate table; the gem types
  of a tier, scrolling in GemScroll; the picked type broken down or crafted by a quantity typed in its field or set by
  -, + and MAX.
- WBP_Gems (UGeoGemsWidget): title, page tabs and shards over a PageSwitcher holding both pages, BACK under them.
- WBP_PauseMenu: GemsWidget beside CharacterWidget, collapsed.
Every list, tab and row the widgets build at runtime is a WBP_ListRow (RowClass).

Never overwrites a hand edit: the widgets are built only when created, the pause menu only gains what it lacks, and
every value goes through asset_guard.write (kept values are listed in AI/Output/gems_page.txt).
Usage: run via MCP execute_script, after character_sheet.py (whose GEMS button opens it).
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
wings_hud = character_sheet.wings_hud
asset_guard = wings_hud.asset_guard
add, write_slot, text = wings_hud.add, wings_hud.write_slot, wings_hud.text
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


def build_loadout():
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
        add(wbp, path, unreal.VerticalBox, "TotalsBox", "DetailColumn")
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "row_class",
                          unreal.load_class(None, ROW_CLASS))
        wings_hud.finish(wbp)
    wbp = unreal.load_asset(path)
    asset_guard.write(path, UTIL.find_widget(wbp, "TotalsTitleText"), "auto_wrap_text", True)
    wings_hud.scrolled(wbp, path, "StackGrid", "StackScroll")
    wings_hud.scrolled(wbp, path, "TotalsBox", "TotalsScroll")
    wings_hud.finish(wbp)
    return wbp


def build_forge():
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
        _, rate_slot = add(wbp, path, unreal.VerticalBox, "RateBox", "LeftBox")
        padded(path, rate_slot, unreal.Margin(0, 12, 0, 0))

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
    wings_hud.scrolled(wbp, path, "GemListBox", "GemScroll")
    wings_hud.finish(wbp)
    return wbp


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

        _, footer_slot = add(wbp, path, unreal.HorizontalBox, "Footer", "GemsBody")
        padded(path, footer_slot, unreal.Margin(0, 16, 0, 0))
        button(wbp, path, "BackButton", "Footer", "BACK")
        spacer(wbp, path, "FooterSpacer", "Footer")
        hint_slot = label(wbp, path, "HintText", "Footer", "ESC  BACK")
        write_slot(path, hint_slot, {"vertical_alignment": CENTER})
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "row_class",
                          unreal.load_class(None, ROW_CLASS))
        wings_hud.finish(wbp)
    return unreal.load_asset(path)


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


def run():
    add_to_pause_menu(build_gems(build_loadout(), build_forge()))
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "gems_page.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
