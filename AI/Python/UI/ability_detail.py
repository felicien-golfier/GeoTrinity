"""
The ability drawer, after the Rail design's A-AbilityDetail board: picking an ability on the character sheet slides it
in from the right over a scrim (character_sheet.py puts both on the sheet).
- WBP_AbilityDetail (UGeoAbilityDetailWidget), in /Game/HUD/InGameMenu, built once: DrawerWidth > DrawerFrame > icon
  tile, name, key cap and timing; the spell clip's place (WIP); then, scrolling, the description, Reload buff lines and
  StatGrid. StatFrameStyle set. An existing drawer loses the CLOSE / PREVIOUS / NEXT buttons of its first version
  (a click outside closes it now) and takes the current DRAWER_WIDTH.

Never overwrites a hand edit: every value goes through asset_guard.write (kept values are listed in
AI/Output/ability_detail.txt).
Usage: run via MCP execute_script, after ability_page.py and before character_sheet.py.
"""
import importlib
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import ability_page
import wings_hud

ability_page = importlib.reload(ability_page)
wings_hud = importlib.reload(wings_hud)
asset_guard = ability_page.asset_guard
add, write_slot, frame_values = ability_page.add, ability_page.write_slot, ability_page.frame_values
text = wings_hud.text

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
ROLE = unreal.GeoTextRole
CENTER = unreal.VerticalAlignment.V_ALIGN_CENTER
FILL = unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)
DETAIL_PATH = f"{ability_page.MENU_DIR}/WBP_AbilityDetail"

DRAWER_WIDTH = 640.0
RETIRED_WIDGETS = ("CloseButton", "PreviousButton", "BrowseSpacer", "NextButton", "BrowseRow")
DRAWER_PADDING = unreal.Margin(32, 28, 32, 28)
ICON_SIZE = 38.0
CLIP_HEIGHT = 240.0
SECTION_GAP = 20.0
STAT_GAP = unreal.Margin(3, 3, 3, 3)


def build_detail():
    path = DETAIL_PATH
    wbp = wings_hud.create_widget(path, "GeoAbilityDetailWidget")
    if wbp:
        text_table = unreal.load_asset(ability_page.TEXT_TABLE_PATH)
        meta_table = unreal.load_asset(ability_page.META_TABLE_PATH)
        UTIL.set_root_panel(wbp, unreal.SizeBox, "DrawerWidth")
        for key, value in dict(override_width_override=True, width_override=DRAWER_WIDTH).items():
            asset_guard.write(path, UTIL.find_widget(wbp, "DrawerWidth"), key, value)
        add(wbp, path, unreal.GeoFrame, "CardFrame", "DrawerWidth", frame_values("DA_Frame_Wing", DRAWER_PADDING))
        add(wbp, path, unreal.VerticalBox, "DrawerBody", "CardFrame")

        # Who: icon, name, key and timing.
        add(wbp, path, unreal.HorizontalBox, "TopRow", "DrawerBody")
        _, tile_slot = add(wbp, path, unreal.GeoFrame, "IconFrame", "TopRow",
                           frame_values("DA_Frame_IconTile", ability_page.ICON_TILE_PADDING))
        write_slot(path, tile_slot, {"vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoIconImage, "Icon", "IconFrame", {"size": ICON_SIZE})
        _, name_slot = add(wbp, path, unreal.VerticalBox, "NameColumn", "TopRow")
        write_slot(path, name_slot, {"size": FILL, "padding": unreal.Margin(18, 0, 12, 0), "vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoText, "NameText", "NameColumn", text(ROLE.HEADING, "ABILITY"))
        _, meta_slot = add(wbp, path, unreal.HorizontalBox, "MetaRow", "NameColumn")
        write_slot(path, meta_slot, {"padding": unreal.Margin(0, 8, 0, 0)})
        _, key_slot = add(wbp, path, unreal.GeoFrame, "KeyFrame", "MetaRow",
                          frame_values("DA_Frame_KeyCap", ability_page.KEY_PADDING))
        write_slot(path, key_slot, {"vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoText, "KeyText", "KeyFrame", text(ROLE.LABEL, "KEY"))
        _, timing_slot = add(wbp, path, unreal.RichTextBlock, "TimingText", "MetaRow",
                             {"text_style_set": meta_table, "text": unreal.Text("COOLDOWN <Value>0s</>")})
        write_slot(path, timing_slot, {"padding": unreal.Margin(14, 0, 0, 0), "vertical_alignment": CENTER})

        # The spell clip's place, until clips exist.
        _, clip_box_slot = add(wbp, path, unreal.SizeBox, "ClipBox", "DrawerBody",
                               {"override_height_override": True, "height_override": CLIP_HEIGHT})
        write_slot(path, clip_box_slot, {"padding": unreal.Margin(0, SECTION_GAP, 0, 0)})
        add(wbp, path, unreal.GeoFrame, "ClipFrame", "ClipBox", frame_values("DA_Frame_Card", unreal.Margin(0, 0, 0, 0)))
        add(wbp, path, unreal.GeoText, "ClipWipText", "ClipFrame",
            text(ROLE.LABEL, "SPELL CLIP COMING SOON", justification=unreal.TextJustify.CENTER, render_opacity=.55))
        clip_frame = UTIL.find_widget(wbp, "ClipFrame")
        for key, value in dict(horizontal_alignment=unreal.HorizontalAlignment.H_ALIGN_CENTER,
                               vertical_alignment=CENTER).items():
            asset_guard.write(path, clip_frame, key, value)

        # What it does, then its numbers: scrolls when long.
        _, scroll_slot = add(wbp, path, unreal.GeoScrollBox, "DetailScroll", "DrawerBody")
        write_slot(path, scroll_slot, {"size": FILL, "padding": unreal.Margin(0, SECTION_GAP, 0, 0)})
        add(wbp, path, unreal.RichTextBlock, "DescriptionText", "DetailScroll",
            {"text_style_set": text_table, "auto_wrap_text": True, "text": unreal.Text("Description.")})
        add(wbp, path, unreal.VerticalBox, "BuffBox", "DetailScroll")
        _, grid_slot = add(wbp, path, unreal.UniformGridPanel, "StatGrid", "DetailScroll", {"slot_padding": STAT_GAP})
        write_slot(path, grid_slot, {"padding": unreal.Margin(0, SECTION_GAP, 0, 0)})
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "stat_frame_style",
                          unreal.load_asset(f"{ability_page.STYLE_DIR}/DA_Frame_Field"))
        wings_hud.finish(wbp)
    update_detail(unreal.load_asset(path))
    return unreal.load_asset(path)


def update_detail(wbp):
    path = DETAIL_PATH
    for name in RETIRED_WIDGETS:
        if UTIL.find_widget(wbp, name):
            UTIL.remove_widget(wbp, name)
    asset_guard.write(path, UTIL.find_widget(wbp, "DrawerWidth"), "width_override", DRAWER_WIDTH, earlier=(544.0,))
    wings_hud.finish(wbp)


def run():
    build_detail()
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "ability_detail.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
