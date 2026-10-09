"""
The one table every page uses (UGeoTableWidget): WBP_Table carries the look, a page sets its columns.
- WBP_Table, in /Game/HUD: TableBody > HeaderLine (the column captions, built from the instance's Columns) and
  LineScroll > LineBox (the lines of DA_Frame_Cell cells the page adds at runtime).
- place(): puts a WBP_Table on a page in place of the widgets it replaces, and writes its columns on the instance.

Never overwrites a hand edit: WBP_Table is built only when created, a replaced widget goes only when it carries no
hand edit, and every value goes through asset_guard.write.
Usage: imported by character_sheet.py and gems_page.py, which build WBP_Table before their pages; after ui_theme.py.
"""
import importlib
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import wings_hud

wings_hud = importlib.reload(wings_hud)
asset_guard = wings_hud.asset_guard
add, write_slot, frame = wings_hud.add, wings_hud.write_slot, wings_hud.frame

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
TABLE_PATH = "/Game/HUD/WBP_Table"
RIGHT = unreal.HorizontalAlignment.H_ALIGN_RIGHT
FILL_ALIGN = unreal.HorizontalAlignment.H_ALIGN_FILL
FILL = unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)


def build_table():
    path = TABLE_PATH
    wbp = wings_hud.create_widget(path, "GeoTableWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.VerticalBox, "TableBody")
        _, header_slot = add(wbp, path, unreal.HorizontalBox, "HeaderLine", "TableBody")
        write_slot(path, header_slot, {"padding": unreal.Margin(0, 0, 0, 4)})
        _, scroll_slot = add(wbp, path, unreal.GeoScrollBox, "LineScroll", "TableBody")
        write_slot(path, scroll_slot, {"size": FILL})
        add(wbp, path, unreal.VerticalBox, "LineBox", "LineScroll")
        wings_hud.finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "cell_style", frame("DA_Frame_Cell"))
        wings_hud.finish(wbp)
    return unreal.load_asset(path)


def column(caption, width=0.0, alignment=RIGHT):
    """A column: its caption, its width (0 fills what the others leave) and where its caption and cells sit."""
    result = unreal.GeoTableColumn()
    result.set_editor_property("header", unreal.Text(caption))
    result.set_editor_property("width", width)
    result.set_editor_property("alignment", alignment)
    return result


def place(wbp, path, name, parent, columns, replaced=()):
    """name, a WBP_Table holding columns, in parent where the first of replaced was (last in parent without it); the
    replaced widgets go unless edited by hand. Returns (the table's slot, the hand edits that kept a widget)."""
    if not UTIL.find_widget(wbp, name):
        old = next((UTIL.find_widget(wbp, item) for item in replaced if UTIL.find_widget(wbp, item)), None)
        index = old.get_parent().get_child_index(old) if old else -1
        UTIL.construct_widget_in_tree(wbp, unreal.load_asset(TABLE_PATH).generated_class(), name, True)
        UTIL.attach_widget(wbp, parent, name, index)
    kept = wings_hud.remove_unless_edited(wbp, path, list(replaced))
    table = UTIL.find_widget(wbp, name)
    asset_guard.write(path, table, "columns", columns)
    return table.get_editor_property("slot"), kept
