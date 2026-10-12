"""
The passive tiles of the ability bar: WBP_PassiveSlot and the PassiveBox left of the ability slots.
- WBP_PassiveSlot: a copy of WBP_AbilitySlot (icon, frame, cooldown sweep, countdown) without the key label and the
  deploy count, square at TILE. Parent UGeoAbilitySlotWidget, so the passive tile is the ability slot's own widget.
- WBP_AbilityBar: Row (HorizontalBox) holding PassiveBox (a vertical WrapBox wrapping every 2 tiles) then SlotBox, and
  PassiveSlotWidgetClass pointing at WBP_PassiveSlot.

Height invariant: 2 tiles + gap = the ability icon + its key label (Source/GeoTrinityUI/Public/HUD/AbilityBar/CLAUDE.md):
retune TILE with the key label's height.
Never overwrites a hand edit: every value goes through asset_guard.write. Usage: headless
UnrealEditor-Win64-DebugGame-Cmd.exe GeoTrinity.uproject -run=pythonscript -script=<this file> -unattended -nullrhi -NoP4; the report
lands in AI/Output/passive_bar.txt. Run after wings_hud.py.
"""
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import ability_page

asset_guard = ability_page.asset_guard
add = ability_page.add
write_slot = ability_page.write_slot

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
FOLDER = "/Game/HUD/AbilityBar"
SLOT_PATH = FOLDER + "/WBP_AbilitySlot"
PASSIVE_PATH = FOLDER + "/WBP_PassiveSlot"
BAR_PATH = FOLDER + "/WBP_AbilityBar"
TILE = 50.0
GAP = 6.0
COUNTDOWN_FONT_SIZE = 14
# The ability slot's key label subtree and its deploy count: not part of a passive tile.
UNUSED = ["KeyText", "KeyScale", "KeyWidth", "CountText"]


def finish(wbp):
    UTIL.commit_tree(wbp)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)


def build_passive_slot():
    if not unreal.EditorAssetLibrary.does_asset_exist(PASSIVE_PATH):
        unreal.EditorAssetLibrary.duplicate_asset(SLOT_PATH, PASSIVE_PATH)
        asset_guard.created(PASSIVE_PATH)
    wbp = unreal.load_asset(PASSIVE_PATH)
    for name in UNUSED:
        if UTIL.find_widget(wbp, name):
            UTIL.remove_widget(wbp, name)
    square = UTIL.find_widget(wbp, "Square")
    # Setters raise the override flag: the SizeBox only honours its override through them.
    square.set_editor_property("override_width_override", True)
    square.set_editor_property("override_height_override", True)
    asset_guard.write(PASSIVE_PATH, square, "width_override", TILE)
    asset_guard.write(PASSIVE_PATH, square, "height_override", TILE)
    countdown = UTIL.find_widget(wbp, "CountdownText")
    font = countdown.get_editor_property("font")
    font.size = COUNTDOWN_FONT_SIZE
    asset_guard.write(PASSIVE_PATH, countdown, "font", font)
    finish(wbp)
    return wbp


def build_bar(passive_wbp):
    wbp = unreal.load_asset(BAR_PATH)
    slot_box = UTIL.find_widget(wbp, "SlotBox")
    box_slot = slot_box.get_editor_property("slot")
    if not UTIL.find_widget(wbp, "Row"):
        root = slot_box.get_parent().get_name()
        row, row_slot = add(wbp, BAR_PATH, unreal.HorizontalBox, "Row", root)
        write_slot(BAR_PATH, row_slot, {
            "horizontal_alignment": box_slot.get_editor_property("horizontal_alignment"),
            "vertical_alignment": box_slot.get_editor_property("vertical_alignment")})
        UTIL.attach_widget(wbp, "Row", "SlotBox")
    created = not UTIL.find_widget(wbp, "PassiveBox")
    passive_box, _ = add(wbp, BAR_PATH, unreal.WrapBox, "PassiveBox", "Row")
    if created:
        # First in the row: left of SlotBox.
        UTIL.attach_widget(wbp, "Row", "PassiveBox", 0)
    passive_slot = passive_box.get_editor_property("slot")
    for key, value in {
        "orientation": unreal.Orientation.ORIENT_VERTICAL,
        "explicit_wrap_size": True,
        "wrap_size": 2 * TILE + GAP + 2.0,
        "inner_slot_padding": unreal.Vector2D(GAP, GAP),
    }.items():
        asset_guard.write(BAR_PATH, passive_box, key, value)
    write_slot(BAR_PATH, passive_slot, {
        "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_BOTTOM,
        "padding": unreal.Margin(0, 0, GAP, 0)})
    defaults = unreal.get_default_object(wbp.generated_class())
    asset_guard.write(BAR_PATH, defaults, "passive_slot_widget_class", passive_wbp.generated_class())
    finish(wbp)


def run():
    build_bar(build_passive_slot())
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "passive_bar.txt")
    open(output, "w").write("\n".join(["OK"] + ["kept by hand: " + line for line in asset_guard.report()]))


try:
    run()
except Exception as error:
    open(os.path.join(unreal.Paths.project_dir(), "AI", "Output", "passive_bar.txt"), "w").write("ERR %r" % error)
