"""
Tutorial HUD widgets: WBP_CatLine (rich text, Bold row + shake decorator), WBP_QuestCard, and their classes on the HUD.
Usage: headless (AI/Commands.md, DebugGame -Cmd) or via MCP execute_script, after ui_theme.py. Re-run-safe: an existing
widget is kept, every value goes through asset_guard. Results in AI/Output/tutorial_hud.txt.
"""
import importlib
import json
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
asset_guard = wings_hud.asset_guard
add, write_slot = wings_hud.add, wings_hud.write_slot

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
ROLE = unreal.GeoTextRole
H = unreal.HorizontalAlignment
V = unreal.VerticalAlignment
HUD_DIR = "/Game/HUD"
CAT_LINE_PATH = f"{HUD_DIR}/Tutorial/WBP_CatLine"
QUEST_CARD_PATH = f"{HUD_DIR}/Tutorial/WBP_QuestCard"
CAT_TABLE_PATH = f"{HUD_DIR}/Style/DT_CatLine"
HUD_BLUEPRINT_PATH = f"{HUD_DIR}/BP_GeoHudMain"

BOLD_COLOR = "FFC840"
LINE_WIDTH = 900.0
LINE_PADDING = unreal.Margin(0, 0, 0, 140)
SKIP_HINT_PADDING = unreal.Margin(0, -8, -8, 0)
CARD_PADDING =unreal.Margin(0, 160, 40, 0)
CARD_GAP = unreal.Margin(0, 0, 0, 6)


def build_cat_table():
    """Default and Bold rows in the Body role's font; only when the table is new."""
    if unreal.EditorAssetLibrary.does_asset_exist(CAT_TABLE_PATH):
        return unreal.load_asset(CAT_TABLE_PATH)
    folder, name = CAT_TABLE_PATH.rsplit("/", 1)
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.RichTextStyleRow.static_struct())
    table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.DataTable, factory)
    theme = unreal.load_asset(f"{ability_page.STYLE_DIR}/DA_UITheme")
    text_style = theme.get_editor_property("text_styles")[ROLE.BODY]
    font_info = text_style.get_editor_property("font")
    body_color = text_style.get_editor_property("color").get_editor_property("specified_color")
    rows = [ability_page.style_row_json("Default", font_info, body_color),
            ability_page.style_row_json("Bold", font_info, ability_page.srgb(BOLD_COLOR))]
    unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows))
    unreal.EditorAssetLibrary.save_loaded_asset(table)
    return table


def build_cat_line():
    wbp = wings_hud.create_widget(CAT_LINE_PATH, "GeoCatLineWidget")
    if not wbp:
        return
    path = CAT_LINE_PATH
    UTIL.set_root_panel(wbp, unreal.Overlay, "Root")
    _, frame_slot = add(wbp, path, unreal.GeoFrame, "LineFrame", "Root",
                        {"frame_style": wings_hud.frame("DA_Frame_Panel"), "padding": unreal.Margin(24, 16, 24, 16)})
    write_slot(path, frame_slot, {"horizontal_alignment": H.H_ALIGN_CENTER, "vertical_alignment": V.V_ALIGN_BOTTOM,
                                  "padding": LINE_PADDING})
    add(wbp, path, unreal.Overlay, "LineStack", "LineFrame")
    add(wbp, path, unreal.HorizontalBox, "FaceRow", "LineStack")
    _, hint_slot = add(wbp, path, unreal.GeoFrame, "SkipHint", "LineStack",
                       {"frame_style": wings_hud.frame("DA_Frame_KeyCap"), "padding": unreal.Margin(8, 1, 8, 1),
                        "visibility": unreal.SlateVisibility.HIT_TEST_INVISIBLE})
    write_slot(path, hint_slot, {"horizontal_alignment": H.H_ALIGN_RIGHT, "vertical_alignment": V.V_ALIGN_TOP,
                                 "padding": SKIP_HINT_PADDING})
    add(wbp, path, unreal.GeoText, "SkipKey", "SkipHint", {"role": ROLE.LABEL, "text": unreal.Text("F")})
    _, face_slot = add(wbp, path, unreal.GeoCatFaceWidget, "CatFace", "FaceRow")
    write_slot(path, face_slot, {"padding": unreal.Margin(0, 0, 16, 0), "vertical_alignment": V.V_ALIGN_CENTER})
    add(wbp, path, unreal.VerticalBox, "LineColumn", "FaceRow")
    add(wbp, path, unreal.GeoText, "CatName", "LineColumn", {"role": ROLE.LABEL, "text": unreal.Text("CAT")})
    add(wbp, path, unreal.SizeBox, "LineWidth", "LineColumn",
        {"override_width_override": True, "width_override": LINE_WIDTH})
    line, _ = add(wbp, path, unreal.RichTextBlock, "LineText", "LineWidth", {"auto_wrap_text": True})
    line.set_editor_property("text_style_set", build_cat_table())
    line.set_editor_property("decorator_classes", [unreal.GeoShakeTextDecorator])
    wings_hud.finish(wbp)


def build_quest_card():
    wbp = wings_hud.create_widget(QUEST_CARD_PATH, "GeoQuestCardWidget")
    if not wbp:
        return
    path = QUEST_CARD_PATH
    UTIL.set_root_panel(wbp, unreal.Overlay, "Root")
    _, frame_slot = add(wbp, path, unreal.GeoFrame, "CardFrame", "Root",
                        {"frame_style": wings_hud.frame("DA_Frame_Panel"), "padding": unreal.Margin(20, 14, 20, 14)})
    write_slot(path, frame_slot, {"horizontal_alignment": H.H_ALIGN_RIGHT, "vertical_alignment": V.V_ALIGN_TOP,
                                  "padding": CARD_PADDING})
    add(wbp, path, unreal.VerticalBox, "CardBox", "CardFrame")
    for name, role, text in [("TitleText", ROLE.HEADING, "QUEST"), ("CountText", ROLE.BODY, "KILLED 0/0"),
                             ("TimerText", ROLE.LABEL, "")]:
        _, slot = add(wbp, path, unreal.GeoText, name, "CardBox", {"role": role, "text": unreal.Text(text)})
        write_slot(path, slot, {"padding": CARD_GAP})
    wings_hud.finish(wbp)


def set_hud_classes():
    """CatLineWidgetClass / QuestCardWidgetClass on the HUD Blueprint's defaults."""
    hud = unreal.load_asset(HUD_BLUEPRINT_PATH)
    unreal.BlueprintEditorLibrary.compile_blueprint(hud)
    default = unreal.get_default_object(hud.generated_class())
    asset_guard.write(HUD_BLUEPRINT_PATH, default, "cat_line_widget_class",
                      unreal.load_asset(CAT_LINE_PATH).generated_class())
    asset_guard.write(HUD_BLUEPRINT_PATH, default, "quest_card_widget_class",
                      unreal.load_asset(QUEST_CARD_PATH).generated_class())
    unreal.EditorAssetLibrary.save_loaded_asset(hud)


def run():
    build_cat_line()
    build_quest_card()
    set_hud_classes()
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "tutorial_hud.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
