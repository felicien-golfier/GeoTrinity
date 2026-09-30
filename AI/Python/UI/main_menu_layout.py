"""Centre the main menu's button column on screen and give every button the same gap; reports to AI/Output/main_menu_layout.txt."""
import os
import unreal

MAIN_MENU_PATH = "/Game/HUD/MainMenu/WBP_MainMenuWidget"
BUTTONS_BOX_NAME = "VB_MainButtons"
BUTTON_GAP = 16.0
REPORT = os.path.join((unreal.Paths.project_dir() + "AI/Output/"), "main_menu_layout.txt")

CENTER = unreal.Anchors(unreal.Vector2D(0.5, 0.5), unreal.Vector2D(0.5, 0.5))


def _describe(widget):
    slot = widget.get_editor_property("slot")
    line = f"{widget.get_name()} ({widget.get_class().get_name()})"
    canvas_slot = slot if isinstance(slot, unreal.CanvasPanelSlot) else None
    box_slot = slot if isinstance(slot, unreal.VerticalBoxSlot) else None
    if canvas_slot:
        line += f" canvas anchors={canvas_slot.get_anchors()} align={canvas_slot.get_alignment()}" \
                f" offsets={canvas_slot.get_offsets()} auto={canvas_slot.get_auto_size()}"
    if box_slot:
        line += f" padding={box_slot.get_editor_property('padding')}" \
                f" halign={box_slot.get_editor_property('horizontal_alignment')}"
    return line


def _canvas_child_holding(widget):
    """The widget itself or the ancestor that sits directly on a canvas — the one to centre."""
    while widget:
        slot = widget.get_editor_property("slot")
        if isinstance(slot, unreal.CanvasPanelSlot):
            return widget
        widget = slot.get_editor_property("parent") if slot else None
    return None


def layout_main_menu():
    lines = []
    bp = unreal.load_asset(MAIN_MENU_PATH)
    util = unreal.GeoWidgetBuilderUtil
    box = util.find_widget(bp, BUTTONS_BOX_NAME)

    lines.append("BEFORE")
    lines += [_describe(child) for child in box.get_all_children()]

    column = _canvas_child_holding(box)
    lines.append("centred: " + _describe(column))
    canvas_slot = column.get_editor_property("slot")
    canvas_slot.set_anchors(CENTER)
    canvas_slot.set_alignment(unreal.Vector2D(0.5, 0.5))
    canvas_slot.set_auto_size(True)
    canvas_slot.set_offsets(unreal.Margin(0, 0, 0, 0))

    # Spacers made the rhythm uneven; the gap lives on the button slots instead.
    for child in box.get_all_children():
        if isinstance(child, unreal.Spacer):
            util.remove_widget(bp, child.get_fname())

    for child in box.get_all_children():
        box_slot = child.get_editor_property("slot")
        box_slot.set_padding(unreal.Margin(0, BUTTON_GAP * 0.5, 0, BUTTON_GAP * 0.5))
        box_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        box_slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        box_slot.set_size(unreal.SlateChildSize(1.0, unreal.SlateSizeRule.AUTOMATIC))

    util.commit_tree(bp)

    lines.append("AFTER")
    lines.append(_describe(column))
    lines += [_describe(child) for child in box.get_all_children()]
    with open(REPORT, "w") as handle:
        handle.write("\n".join(lines))


layout_main_menu()
