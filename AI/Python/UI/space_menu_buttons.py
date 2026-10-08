"""
Re-lays every widget Blueprint whose menu buttons sit unevenly: commit_tree spaces the buttons of each vertical and
horizontal box by the theme's MenuButtonGap. Run after changing the gap, or on widgets no builder has committed since
the rule existed. Only widgets with an uneven stack are committed, so nothing else is re-saved.
Writes AI/Output/space_menu_buttons.txt: each widget re-laid, with its stacks' gaps before.
Usage: run via MCP execute_script.
"""
import os

import unreal

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
THEME_PATH = "/Game/HUD/Style/DA_UITheme"


def gaps(buttons):
    """The space between each pair of neighbouring buttons of one box, in order."""
    vertical = isinstance(buttons[0].get_parent(), unreal.VerticalBox)
    paddings = [button.get_editor_property("slot").get_editor_property("padding") for button in buttons]
    if vertical:
        return [before.bottom + after.top for before, after in zip(paddings, paddings[1:])]
    return [before.right + after.left for before, after in zip(paddings, paddings[1:])]


def uneven_stacks():
    """{widget blueprint path: [gaps of each uneven stack]}, from every menu button in a /Game/HUD widget tree."""
    theme_gap = unreal.load_asset(THEME_PATH).get_editor_property("menu_button_gap")
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in registry.get_assets_by_path("/Game/HUD", recursive=True):
        if str(data.asset_class_path.asset_name) == "WidgetBlueprint":
            data.get_asset()
    stacks = {}
    for button in unreal.ObjectIterator(unreal.GeoMenuButton):
        path = button.get_path_name()
        parent = button.get_parent()
        if ":WidgetTree." in path and not path.startswith("/Engine/Transient") and isinstance(
                parent, (unreal.VerticalBox, unreal.HorizontalBox)):
            stacks.setdefault((path.split(".")[0], parent.get_name()), parent)
    uneven = {}
    for (asset, _), box in stacks.items():
        buttons = [box.get_child_at(i) for i in range(box.get_children_count())
                   if isinstance(box.get_child_at(i), unreal.GeoMenuButton)]
        if len(buttons) > 1:
            box_gaps = gaps(buttons)
            if any(abs(box_gap - theme_gap) > 0.01 for box_gap in box_gaps):
                uneven.setdefault(asset, []).append(box_gaps)
    return uneven


def run():
    uneven = uneven_stacks()
    for asset in uneven:
        UTIL.commit_tree(unreal.load_asset(asset))
    lines = [f"{asset}: gaps before {stacks}" for asset, stacks in uneven.items()]
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "space_menu_buttons.txt")
    open(output, "w", encoding="utf-8").write("\n".join(lines or ["all even"]) + "\n")


if __name__ == "__main__":
    run()
