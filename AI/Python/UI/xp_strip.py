"""
The XP strip of the player's wing: WBP_XpStrip (UGeoXpStripWidget) under the health meter in WBP_PlayerWing.
- DA_Meter_Xp: written by wings_hud.build_meter_styles.
- WBP_XpStrip in /Game/HUD/Wings: a row of LevelText, EchoLevelText (each in a SizeBox with a minimum width, so the echo slot
  holds its room while Hidden) then XpMeter filling the rest; built once.
- WBP_PlayerWing: XpStrip appended to Info under the health stack.

Never overwrites a hand edit: new widgets are built only when created, every value goes through asset_guard.write.
Usage: headless UnrealEditor-Win64-DebugGame-Cmd.exe GeoTrinity.uproject -run=pythonscript -script=<this file>
-unattended -nullrhi -NoP4, after wings_hud.py; the report lands in AI/Output/xp_strip.txt.
"""
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import wings_hud

asset_guard = wings_hud.asset_guard
add = wings_hud.add
write_slot = wings_hud.write_slot
UTIL = wings_hud.UTIL
ROLE = wings_hud.ROLE

STRIP_PATH = f"{wings_hud.WINGS_DIR}/WBP_XpStrip"
WING_PATH = f"{wings_hud.WINGS_DIR}/WBP_PlayerWing"
# Least room each number keeps (a SizeBox grows past it with its text, so a number never runs into the bar): two
# digits for the class level, the middle point and the echo level's three digits for the echo.
LEVEL_MIN_WIDTH = 26.0
ECHO_MIN_WIDTH = 56.0
# The wing's HealthStack keeps room under the bar for the health text's overhang (12 above and below); the strip
# claws most of it back so the two read as one block.
STRIP_GAP_ABOVE = 2.0
HEALTH_STACK_PADDING = unreal.Margin(0, 12, 0, 2)


def number(wbp, path, box, name, min_width):
    """A SizeBox with a minimum width holding a left-aligned mono number; its colour is the strip widget's LevelColor / EchoLevelColor."""
    _, box_slot = add(wbp, path, unreal.SizeBox, box, "StripRow",
                      {"override_min_desired_width": True, "min_desired_width": min_width})
    write_slot(path, box_slot, {"vertical_alignment": wings_hud.CENTER})
    add(wbp, path, unreal.GeoText, name, box, wings_hud.text(ROLE.MONO, "0"))


def build_strip():
    wbp = wings_hud.create_widget(STRIP_PATH, "GeoXpStripWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.HorizontalBox, "StripRow")
        number(wbp, STRIP_PATH, "LevelBox", "LevelText", LEVEL_MIN_WIDTH)
        number(wbp, STRIP_PATH, "EchoBox", "EchoLevelText", ECHO_MIN_WIDTH)
        add(wbp, STRIP_PATH, unreal.GeoMeter, "XpMeter", "StripRow",
            {"meter_style": wings_hud.meter("DA_Meter_Xp"), "fill": .6})
        wings_hud.finish(wbp)
    wbp = unreal.load_asset(STRIP_PATH)
    for box, min_width in (("LevelBox", LEVEL_MIN_WIDTH), ("EchoBox", ECHO_MIN_WIDTH)):
        size_box = UTIL.find_widget(wbp, box)
        asset_guard.write(STRIP_PATH, size_box, "override_width_override", False)
        asset_guard.write(STRIP_PATH, size_box, "override_min_desired_width", True)
        asset_guard.write(STRIP_PATH, size_box, "min_desired_width", min_width)
    write_slot(STRIP_PATH, UTIL.find_widget(wbp, "XpMeter").get_editor_property("slot"),
               {"size": wings_hud.FILL, "horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_FILL,
                "vertical_alignment": wings_hud.CENTER})
    wings_hud.finish(wbp)
    return wbp


def place_in_wing(strip):
    wing = unreal.load_asset(WING_PATH)
    if not UTIL.find_widget(wing, "XpStrip"):
        asset_guard.created(WING_PATH)
        UTIL.construct_widget_in_tree(wing, strip.generated_class(), "XpStrip", True)
        UTIL.attach_widget(wing, "Info", "XpStrip")
        # A new slot holds no hand edit, but the ledger may still list the removed widget's slot of the same name.
        UTIL.find_widget(wing, "XpStrip").get_editor_property("slot").set_editor_property(
            "padding", unreal.Margin(0, STRIP_GAP_ABOVE, 0, 0))
    write_slot(WING_PATH, UTIL.find_widget(wing, "HealthStack").get_editor_property("slot"),
               {"padding": HEALTH_STACK_PADDING})
    write_slot(WING_PATH, UTIL.find_widget(wing, "XpStrip").get_editor_property("slot"),
               {"padding": unreal.Margin(0, STRIP_GAP_ABOVE, 0, 0),
                "horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_FILL})
    wings_hud.finish(wing)


def run():
    wings_hud.build_meter_styles()
    place_in_wing(build_strip())
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "xp_strip.txt")
    open(output, "w").write("\n".join(["OK"] + ["kept by hand: " + line for line in asset_guard.report()]))


try:
    run()
except Exception as error:
    open(os.path.join(unreal.Paths.project_dir(), "AI", "Output", "xp_strip.txt"), "w").write("ERR %r" % error)
