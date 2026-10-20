"""
Tutorial entry in the menus: WBP_TutorialPrompt (the Cat's first-launch page, bare window), TutorialButton and the prompt
page in the main menu, LeaveTutorialButton in the pause menu.
Usage: headless (AI/Commands.md, DebugGame -Cmd) or via MCP execute_script, after menu_pages.py. Re-run-safe: a widget
that exists is kept, every value goes through asset_guard. Results in AI/Output/tutorial_menu.txt.
"""
import importlib
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import rail_style
import wings_hud

rail_style = importlib.reload(rail_style)
wings_hud = importlib.reload(wings_hud)
asset_guard = wings_hud.asset_guard
add, write_slot = wings_hud.add, wings_hud.write_slot

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
HUD = "/Game/HUD"
PROMPT_PATH = f"{HUD}/MainMenu/WBP_TutorialPrompt"
MENU_FRAME_PATH = f"{HUD}/WBP_MenuFrame"
MAIN_MENU_PATH = f"{HUD}/MainMenu/WBP_MainMenuWidget"
PAUSE_PATH = f"{HUD}/InGameMenu/WBP_PauseMenu"
BUTTON_CLASS = f"{HUD}/WBP_GeoButton.WBP_GeoButton_C"

PROMPT_WIDTH = 720.0
# Placeholders until the Tutorial Board page "Prompt" is validated (AI/Pending/Tutorial.md, open design).
PROMPT_TITLE = "A WORD, ADVENTURER"
PROMPT_BODY = "You should do the tutorial. Skipping it is fine, if you enjoy being incompetent and ignorant."
DO_IT_LABEL = "Do the tutorial"
SKIP_LABEL = "I know best"
TEXT_GAP = unreal.Margin(0, 0, 0, 24)

H = unreal.HorizontalAlignment
V = unreal.VerticalAlignment
ROLE = unreal.GeoTextRole
skipped = []


def build_prompt():
    """The Cat's prompt page, in the bare window: no Back, no close cross."""
    wbp = wings_hud.create_widget(PROMPT_PATH, "GeoTutorialPromptWidget")
    if not wbp:
        return
    path = PROMPT_PATH
    menu_frame = unreal.load_asset(MENU_FRAME_PATH)
    UTIL.construct_widget_in_tree(wbp, menu_frame.generated_class(), "MenuFrame", True)
    UTIL.set_root_widget(wbp, "MenuFrame")
    UTIL.construct_widget_in_tree(wbp, unreal.Overlay, "PromptContent", True)
    UTIL.set_named_slot_content(wbp, "MenuFrame", "ContentSlot", "PromptContent")
    box, box_slot = add(wbp, path, unreal.SizeBox, "PromptWidth", "PromptContent",
                        {"override_width_override": True, "width_override": PROMPT_WIDTH})
    write_slot(path, box_slot, {"horizontal_alignment": H.H_ALIGN_CENTER, "vertical_alignment": V.V_ALIGN_CENTER})
    add(wbp, path, unreal.VerticalBox, "PromptBox", "PromptWidth")
    _, title_slot = add(wbp, path, unreal.GeoText, "PromptTitle", "PromptBox",
                        {"role": ROLE.TITLE, "text": unreal.Text(PROMPT_TITLE)})
    write_slot(path, title_slot, {"padding": TEXT_GAP})
    _, body_slot = add(wbp, path, unreal.GeoText, "PromptBody", "PromptBox",
                       {"role": ROLE.BODY, "text": unreal.Text(PROMPT_BODY), "auto_wrap_text": True})
    write_slot(path, body_slot, {"padding": TEXT_GAP})
    button_class = unreal.load_class(None, BUTTON_CLASS)
    add(wbp, path, button_class, "DoItButton", "PromptBox", {"label": unreal.Text(DO_IT_LABEL)})
    add(wbp, path, button_class, "SkipButton", "PromptBox", {"label": unreal.Text(SKIP_LABEL)})
    wings_hud.finish(wbp)


def add_main_menu_entries():
    """TutorialButton above the leaderboard's, and the prompt as a page like the others."""
    wbp = unreal.load_asset(MAIN_MENU_PATH)
    if not UTIL.find_widget(wbp, "TutorialButton"):
        before = UTIL.find_widget(wbp, "LeaderboardButton")
        box = before.get_parent()
        button = UTIL.construct_widget_in_tree(wbp, unreal.load_class(None, BUTTON_CLASS), "TutorialButton", True)
        button.set_editor_property("label", unreal.Text("Tutorial"))
        UTIL.attach_widget(wbp, box.get_name(), "TutorialButton", box.get_child_index(before))
    if not UTIL.find_widget(wbp, "TutorialPromptWidget"):
        sibling = UTIL.find_widget(wbp, "LeaderboardWidget")
        page_class = unreal.load_asset(PROMPT_PATH).generated_class()
        UTIL.construct_widget_in_tree(wbp, page_class, "TutorialPromptWidget", True)
        slot = UTIL.attach_widget(wbp, sibling.get_parent().get_name(), "TutorialPromptWidget")
        menu_pages = importlib.import_module("menu_pages")
        menu_pages.fill(slot)
        UTIL.find_widget(wbp, "TutorialPromptWidget").set_visibility(unreal.SlateVisibility.COLLAPSED)
    wings_hud.finish(wbp)


def add_pause_entry():
    """LeaveTutorialButton above Return to main menu; the menu hides it outside the tutorial map."""
    wbp = unreal.load_asset(PAUSE_PATH)
    if not UTIL.find_widget(wbp, "LeaveTutorialButton"):
        before = UTIL.find_widget(wbp, "ReturnToMainMenuButton")
        box = before.get_parent()
        button = UTIL.construct_widget_in_tree(wbp, unreal.load_class(None, BUTTON_CLASS), "LeaveTutorialButton", True)
        button.set_editor_property("label", unreal.Text("Leave tutorial"))
        UTIL.attach_widget(wbp, box.get_name(), "LeaveTutorialButton", box.get_child_index(before))
    wings_hud.finish(wbp)


def run():
    build_prompt()
    add_main_menu_entries()
    add_pause_entry()
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "tutorial_menu.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["skipped: " + line for line in skipped]
                                                        + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
