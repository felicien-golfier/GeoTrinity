"""
Checks the UI on screen in a running session for the two layout faults a screenshot hides in a long page: texts drawn
over each other, and widgets spilling out of their area.
- Overlap: two visible texts of one screen layer (one widget added to the viewport) whose drawn rectangles intersect;
  each rectangle is first cut to the scroll boxes it sits in, since a scroll box clips what it holds.
- Spill: a visible widget reaching past the screen, or past the user widget it belongs to.
Every visible widget of every live user widget in the session's worlds is checked; collapsed and hidden ones, any
inside them, the pages a widget switcher is not showing, and those drawn over the world on purpose (FREE_FLOATING) are
skipped. Run it after opening each page, after a frame has drawn.

Writes AI/Output/pie_layout_audit.txt: OK, or one line per fault naming the widgets, their owner and their rectangles.
Usage: run via MCP execute_script during PIE. TOLERANCE (px) ignores antialiasing-sized touches.
"""
import os

import unreal

TOLERANCE = 2.0
TEXT_CLASSES = (unreal.TextBlock, unreal.RichTextBlock)
HIDDEN = (unreal.SlateVisibility.COLLAPSED, unreal.SlateVisibility.HIDDEN)
UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
# Drawn over the world on purpose, anywhere on screen: floating damage numbers.
FREE_FLOATING = ("WBP_DamageNumber_C",)


def owner_of(widget):
    """The user widget whose tree holds widget, or None for a user widget placed directly on the viewport."""
    outer = widget.get_outer()
    while outer and not isinstance(outer, unreal.UserWidget):
        outer = outer.get_outer()
    return outer


def chain(widget):
    """widget, then every panel and user widget it sits in, up to the viewport."""
    while widget:
        yield widget
        parent = widget.get_parent()
        widget = parent if parent else owner_of(widget)


def on_inactive_page(link):
    """link is a page of a widget switcher other than its active one: kept visible, but never painted."""
    parent = link.get_parent()
    return isinstance(parent, unreal.WidgetSwitcher) and parent.get_active_widget() != link


def shown(widget):
    return all(link.get_visibility() not in HIDDEN and not on_inactive_page(link) for link in chain(widget))


def rect(widget):
    """Left, top, right, bottom in desktop pixels, where the last frame painted widget."""
    box = UTIL.get_painted_rect(widget)
    return box.min.x, box.min.y, box.max.x, box.max.y


def cut(a, b):
    return max(a[0], b[0]), max(a[1], b[1]), min(a[2], b[2]), min(a[3], b[3])


def empty(r):
    return r[2] - r[0] <= TOLERANCE or r[3] - r[1] <= TOLERANCE


def visible_rect(widget):
    """widget's rectangle cut to every scroll box it sits in."""
    r = rect(widget)
    for link in chain(widget):
        if link is not widget and isinstance(link, unreal.ScrollBox):
            r = cut(r, rect(link))
    return r


def layer(widget):
    """The widget added to the viewport that widget is drawn in: a page over the HUD hides it, so only texts of one
    layer can collide."""
    return list(chain(widget))[-1]


def label(widget):
    owner = owner_of(widget)
    return f"{owner.get_class().get_name() if owner else '-'}.{widget.get_name()}"


def fmt(r):
    return "({:.0f},{:.0f} {:.0f}x{:.0f})".format(r[0], r[1], r[2] - r[0], r[3] - r[1])


def floating(widget):
    return any(link.get_class().get_name() in FREE_FLOATING for link in chain(widget))


def live_widgets():
    for widget in unreal.ObjectIterator(unreal.Widget):
        world = widget.get_world()
        # A play-in-editor world's package carries the UEDPIE_ prefix; the editor's own world never draws game UI.
        if world and "UEDPIE_" in world.get_path_name() and shown(widget) and not floating(widget):
            yield widget


def run():
    widgets = list(live_widgets())
    faults = []

    texts = [(w, visible_rect(w)) for w in widgets if isinstance(w, TEXT_CLASSES)]
    texts = [(w, r) for w, r in texts if not empty(r) and str(w.get_text())]
    for index, (first, first_rect) in enumerate(texts):
        for second, second_rect in texts[index + 1:]:
            if layer(first) is layer(second) and not empty(cut(first_rect, second_rect)):
                faults.append(f"overlap: {label(first)} {fmt(first_rect)} and {label(second)} {fmt(second_rect)}")

    for widget in widgets:
        owner = owner_of(widget)
        if not owner or isinstance(widget, unreal.UserWidget):
            continue
        r, area = visible_rect(widget), rect(owner)
        if empty(r) or empty(area):
            continue
        if (r[0] < area[0] - TOLERANCE or r[1] < area[1] - TOLERANCE or r[2] > area[2] + TOLERANCE
                or r[3] > area[3] + TOLERANCE):
            faults.append(f"spill: {label(widget)} {fmt(r)} out of {owner.get_class().get_name()} {fmt(area)}")

    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "pie_layout_audit.txt")
    open(output, "w", encoding="utf-8").write("\n".join(faults or ["OK"]) + "\n")


if __name__ == "__main__":
    run()
