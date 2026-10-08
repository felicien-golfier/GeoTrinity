"""
The Interface settings panel: WBP_InterfaceSettings (UGeoInterfaceSettingsWidget) with its Show combat stats checkbox,
and its INTERFACE button and panel in WBP_Settings, beside Sound and Key Bindings.
- WBP_InterfaceSettings: built once, in a panel frame; its BackButton is the menu button class WBP_Settings uses.
- WBP_Settings: InterfaceButton after KeyBindingsButton and InterfaceWidget beside SoundWidget, each laid out like its
  sibling; added only when absent.

Never overwrites a hand edit: every value goes through asset_guard.write (kept values are listed in
AI/Output/interface_settings.txt).
Usage: run via MCP execute_script, after the build that adds UGeoInterfaceSettingsWidget and after rail_sub_panels.py.
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
add = ability_page.add
write_slot = ability_page.write_slot
text = wings_hud.text

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
ROLE = unreal.GeoTextRole
CENTER = unreal.VerticalAlignment.V_ALIGN_CENTER
FILL = unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)
SETTINGS_PATH = "/Game/HUD/InGameMenu/WBP_Settings"
PANEL_PATH = "/Game/HUD/InGameMenu/WBP_InterfaceSettings"
SLOT_PROPERTIES = ["padding", "size", "horizontal_alignment", "vertical_alignment", "layout_data", "auto_size",
                   "z_order"]


def sibling_slot_values(sibling):
    """The slot layout of sibling, property by property, for a new widget placed beside it."""
    values = {}
    for name in SLOT_PROPERTIES:
        try:
            values[name] = sibling.get_editor_property("slot").get_editor_property(name)
        except Exception:
            pass
    return values


def add_after(wbp, path, widget_class, name, sibling_name):
    """Constructs name right after sibling_name in its parent, laid out like it; leaves an existing widget of that name
    in place."""
    widget = UTIL.find_widget(wbp, name)
    if not widget:
        sibling = UTIL.find_widget(wbp, sibling_name)
        parent = sibling.get_parent()
        UTIL.construct_widget_in_tree(wbp, widget_class, name, True)
        UTIL.attach_widget(wbp, parent.get_name(), name, parent.get_child_index(sibling) + 1)
        widget = UTIL.find_widget(wbp, name)
        write_slot(path, widget.get_editor_property("slot"), sibling_slot_values(sibling))
    return widget


def build_panel(button_class, back_label):
    path = PANEL_PATH
    wbp = wings_hud.create_widget(path, "GeoInterfaceSettingsWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.GeoFrame, "PanelFrame")
        for key, value in dict(frame_style=wings_hud.frame("DA_Frame_Panel"),
                               padding=unreal.Margin(40, 32, 40, 32)).items():
            asset_guard.write(path, UTIL.find_widget(wbp, "PanelFrame"), key, value)
        add(wbp, path, unreal.VerticalBox, "Root", "PanelFrame")
        _, title_slot = add(wbp, path, unreal.GeoText, "InterfaceLabel", "Root", text(ROLE.HEADING, "Interface"))
        write_slot(path, title_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_CENTER,
                                      "padding": unreal.Margin(0, 0, 0, 24)})
        _, row_slot = add(wbp, path, unreal.HorizontalBox, "CombatStatsRow", "Root")
        write_slot(path, row_slot, {"padding": unreal.Margin(0, 12, 0, 12)})
        _, label_slot = add(wbp, path, unreal.GeoText, "CombatStatsLabel", "CombatStatsRow",
                            text(ROLE.BODY, "Show combat stats"))
        write_slot(path, label_slot, {"size": FILL, "vertical_alignment": CENTER,
                                      "padding": unreal.Margin(0, 0, 24, 0)})
        _, check_slot = add(wbp, path, unreal.GeoCheckBox, "CombatStatsCheckBox", "CombatStatsRow",
                            {"checked_state": unreal.CheckBoxState.CHECKED})
        write_slot(path, check_slot, {"vertical_alignment": CENTER})
        _, back_slot = add(wbp, path, button_class, "BackButton", "Root", {"label": back_label})
        write_slot(path, back_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_FILL,
                                     "padding": unreal.Margin(0, 24, 0, 0)})
        wings_hud.finish(wbp)
    return unreal.load_asset(path)


def run():
    settings = unreal.load_asset(SETTINGS_PATH)
    key_bindings_button = UTIL.find_widget(settings, "KeyBindingsButton")
    button_class = key_bindings_button.get_class()
    back_label = UTIL.find_widget(settings, "BackButton").get_editor_property("label")
    upper = str(key_bindings_button.get_editor_property("label")).isupper()
    panel = build_panel(button_class, back_label)

    button = add_after(settings, SETTINGS_PATH, button_class, "InterfaceButton", "KeyBindingsButton")
    asset_guard.write(SETTINGS_PATH, button, "label", unreal.Text("INTERFACE" if upper else "Interface"))
    asset_guard.write(SETTINGS_PATH, button, "frame_style", key_bindings_button.get_editor_property("frame_style"))
    widget = add_after(settings, SETTINGS_PATH, panel.generated_class(), "InterfaceWidget", "SoundWidget")
    asset_guard.write(SETTINGS_PATH, widget, "visibility", unreal.SlateVisibility.COLLAPSED)
    wings_hud.finish(settings)

    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "interface_settings.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
