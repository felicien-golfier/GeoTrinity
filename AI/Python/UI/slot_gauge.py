"""
The value gauge of the ability slots and passive tiles: the class gauge rising over the icon, with its number.
- DA_Meter_SlotGauge: written by wings_hud.build_meter_styles (Rise, transparent track, white fill the class tints).
- WBP_AbilitySlot / WBP_PassiveSlot: GaugeMeter in Stack right after Icon (under CooldownMeter, fill 0), and GaugeText
  centred over the icon like CountdownText (same class, font and slot alignment, copied from the slot's own countdown).

Never overwrites a hand edit: every value goes through asset_guard.write. Usage: headless
UnrealEditor-Win64-DebugGame-Cmd.exe GeoTrinity.uproject -run=pythonscript -script=<this file> -unattended -nullrhi -NoP4; the report
lands in AI/Output/slot_gauge.txt. Run after wings_hud.py and passive_bar.py.
"""
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import wings_hud

asset_guard = wings_hud.asset_guard
write_slot = wings_hud.write_slot

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
FOLDER = "/Game/HUD/AbilityBar"
SLOT_PATHS = [FOLDER + "/WBP_AbilitySlot", FOLDER + "/WBP_PassiveSlot"]
# Text looks copied from CountdownText so the gauge number and the countdown read as one centre text.
COPIED_TEXT_PROPERTIES = ["font", "color_and_opacity", "justification", "shadow_offset", "shadow_color_and_opacity"]
COPIED_SLOT_PROPERTIES = ["horizontal_alignment", "vertical_alignment", "padding"]


def add_gauge_meter(wbp, path):
    if not UTIL.find_widget(wbp, "GaugeMeter"):
        stack = UTIL.find_widget(wbp, "Stack")
        UTIL.construct_widget_in_tree(wbp, unreal.GeoMeter, "GaugeMeter", True)
        UTIL.attach_widget(wbp, "Stack", "GaugeMeter", stack.get_child_index(UTIL.find_widget(wbp, "Icon")) + 1)
    gauge = UTIL.find_widget(wbp, "GaugeMeter")
    asset_guard.write(path, gauge, "meter_style", wings_hud.meter("DA_Meter_SlotGauge"))
    asset_guard.write(path, gauge, "fill", 0.0)
    write_slot(path, gauge.get_editor_property("slot"),
               {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_FILL,
                "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_FILL})


def add_gauge_text(wbp, path):
    countdown = UTIL.find_widget(wbp, "CountdownText")
    if not UTIL.find_widget(wbp, "GaugeText"):
        UTIL.construct_widget_in_tree(wbp, countdown.get_class(), "GaugeText", True)
        UTIL.attach_widget(wbp, "Stack", "GaugeText")
    gauge_text = UTIL.find_widget(wbp, "GaugeText")
    properties = COPIED_TEXT_PROPERTIES + (["role"] if isinstance(countdown, unreal.GeoText) else [])
    for key in properties:
        asset_guard.write(path, gauge_text, key, countdown.get_editor_property(key))
    asset_guard.write(path, gauge_text, "text", unreal.Text("42%"))
    countdown_slot = countdown.get_editor_property("slot")
    write_slot(path, gauge_text.get_editor_property("slot"),
               {key: countdown_slot.get_editor_property(key) for key in COPIED_SLOT_PROPERTIES})


def run():
    wings_hud.build_meter_styles()
    for path in SLOT_PATHS:
        wbp = unreal.load_asset(path)
        add_gauge_meter(wbp, path)
        add_gauge_text(wbp, path)
        wings_hud.finish(wbp)
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "slot_gauge.txt")
    open(output, "w").write("\n".join(["OK"] + ["kept by hand: " + line for line in asset_guard.report()]))


try:
    run()
except Exception as error:
    open(os.path.join(unreal.Paths.project_dir(), "AI", "Output", "slot_gauge.txt"), "w").write("ERR %r" % error)
