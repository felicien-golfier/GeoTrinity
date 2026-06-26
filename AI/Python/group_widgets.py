"""
Wrap a set of existing canvas children into one new panel anchored on the parent canvas, WITHOUT moving
anything on screen: the new panel is sized to the children's bounding box and each child's offsets are rebased
to that origin. Re-parents children (keeps their name/GUID/graph bindings) rather than re-creating them.

Pass each child's CURRENT canvas offsets (left, top, width, height) — read them first with
GeoWidgetBuilderUtil.inspect_widget_blueprint, since Python cannot read the protected widget tree. Children are
re-parented in list order, which is z-order. Composes the generic GeoWidgetBuilderUtil tree primitives:
construct_widget_in_tree, attach_widget, commit_tree.

Usage: run via MCP execute_script with the editor running and the build exposing the primitives.
"""
import unreal

ZERO_ANCHOR = unreal.Anchors(unreal.Vector2D(0, 0), unreal.Vector2D(0, 0))


def _set_canvas_slot(slot, left, top, width, height):
    canvas_slot = unreal.CanvasPanelSlot.cast(slot)
    canvas_slot.set_anchors(ZERO_ANCHOR)
    canvas_slot.set_offsets(unreal.Margin(left, top, width, height))
    canvas_slot.set_auto_size(False)


def _bounding_box(children):
    """children: list of (name, left, top, width, height). Returns (origin_left, origin_top, width, height)."""
    min_left = min(c[1] for c in children)
    min_top = min(c[2] for c in children)
    max_right = max(c[1] + c[3] for c in children)
    max_bottom = max(c[2] + c[4] for c in children)
    return min_left, min_top, max_right - min_left, max_bottom - min_top


def group_widgets_under_canvas(blueprint_path, parent_panel, group_name, children, group_class=None):
    """
    blueprint_path: e.g. "/Game/HUD/WBP_MainOverlay"
    parent_panel:   name of the existing CanvasPanel currently holding the children
    group_name:     name for the new container (becomes a graph variable)
    children:       list of (child_name, current_left, current_top, width, height) in desired z-order
    group_class:    panel class for the container (default CanvasPanel — preserves pixel offsets exactly)
    """
    group_class = group_class or unreal.CanvasPanel
    bp = unreal.load_asset(blueprint_path)
    util = unreal.GeoWidgetBuilderUtil

    origin_left, origin_top, width, height = _bounding_box(children)

    util.construct_widget_in_tree(bp, group_class, group_name, True)
    _set_canvas_slot(util.attach_widget(bp, parent_panel, group_name, -1),
                     origin_left, origin_top, width, height)

    for name, left, top, w, h in children:
        _set_canvas_slot(util.attach_widget(bp, group_name, name, -1),
                         left - origin_left, top - origin_top, w, h)

    util.commit_tree(bp)
    unreal.log(f"group_widgets.py: grouped {len(children)} widget(s) into '{group_name}' on {blueprint_path}")


# Example call — adjust paths, names, and the inspected offsets for your asset.
group_widgets_under_canvas(
    "/Game/HUD/WBP_MainOverlay",
    "CanvasPanel_21",
    "LifeBarGroup",
    [
        ("ProgressBar_Shield", 16.0, 12.4, 209.1, 28.1),
        ("ProgressBar_Health", 20.0, 16.0, 200.0, 20.0),
        ("HealthText", 50.0, 10.0, 150.0, 32.0),
        ("HealthText_1", 50.0, 10.0, 150.0, 32.0),
        ("HealthText_2", 50.0, 10.0, 150.0, 32.0),
    ],
)
