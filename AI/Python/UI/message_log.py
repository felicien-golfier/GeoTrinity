"""
The message strip of every menu window: UGeoMessageLog "MessageLog" in the bottom row of WBP_MenuFrame, over the
padding, bottom-aligned and filling horizontally, its text starting right of BACK (LeftInset = BACK's width plus a gap).
Layer 1 holds it, so the top level of the main and pause menus and every page, with or without BACK, show the strip.

Never overwrites a hand edit: the widget is added once, every value goes through asset_guard.write.
Usage: headless UnrealEditor-Win64-DebugGame-Cmd.exe GeoTrinity.uproject -run=pythonscript -script=<this file>
-unattended -nullrhi -NoP4, after menu_pages.py; the report lands in AI/Output/message_log.txt.
"""
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import menu_pages
import wings_hud

asset_guard = wings_hud.asset_guard
add = wings_hud.add
write_slot = wings_hud.write_slot
UTIL = wings_hud.UTIL

H = unreal.HorizontalAlignment
V = unreal.VerticalAlignment
GAP_AFTER_BACK = 16.0
# The frame's bottom bar is drawn over the window's lowest pixels; the text sits above it.
ABOVE_FRAME_BAR = 16.0
# Bottom row of the window: BACK's corner inset on the left and bottom, the frame's padding on the right.
SLOT_PADDING = unreal.Margin(menu_pages.BACK_CORNER_INSET.left, 0, menu_pages.FRAME_PADDING.right,
                             menu_pages.BACK_CORNER_INSET.bottom + ABOVE_FRAME_BAR)
VALUES = {"max_lines": 5, "line_height": 15.0, "left_inset": menu_pages.BACK_WIDTH + GAP_AFTER_BACK,
          "lifetime": 4.0, "fade_time": 0.4, "slide_time": 0.25, "text_role": wings_hud.ROLE.LABEL,
          "visibility": unreal.SlateVisibility.HIT_TEST_INVISIBLE}


def build_log():
    path = menu_pages.MENU_FRAME_PATH
    wbp = unreal.load_asset(path)
    _, log_slot = add(wbp, path, unreal.GeoMessageLog, "MessageLog", "FrameOverlay", VALUES)
    write_slot(path, log_slot, {"horizontal_alignment": H.H_ALIGN_FILL, "vertical_alignment": V.V_ALIGN_BOTTOM,
                                "padding": SLOT_PADDING})
    wings_hud.finish(wbp)


def run():
    build_log()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "message_log.txt")
    open(output, "w").write("\n".join(["OK"] + ["kept by hand: " + line for line in asset_guard.report()]))


try:
    run()
except Exception as error:
    open(os.path.join(unreal.Paths.project_dir(), "AI", "Output", "message_log.txt"), "w").write("ERR %r" % error)
