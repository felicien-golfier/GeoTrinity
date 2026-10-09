"""
Every menu page in one window: the same size wherever it opens, BACK at its bottom left, a close cross on its top-right
corner. Back retraces the path taken, the cross leaves every page (UGeoMenuRootWidget, UGeoMenuPageWidget).
- WBP_MenuFrame (UGeoMenuFrameWidget), layer 1, the bare window: FrameBox (the screen inset by the theme's
  MenuFrameMargin, applied in C++) > PanelFrame (DA_Frame_Panel) > ContentSlot, CornerSlot over its top-right corner and
  BackCornerSlot over its bottom-left one.
- WBP_MenuPageFrame (UGeoMenuPageFrameWidget), layer 2: WBP_MenuFrame holding PageSlot, BACK in its bottom-left corner
  and the cross in its top-right one: a Rail button like BACK (CloseFrame in DA_Frame_Button, lit on hover and focus) round the Menu_Remove icon.
- WBP_MenuCluster: a section of the window (DA_Frame_Section) at one width, centred, holding a compact page in its
  NamedSlot "ClusterSlot", so a short form never floats lost in the big window.
- Every page wears WBP_MenuPageFrame as its root, its own content in PageSlot inside PageContent, aligned as PAGES says,
  or in a WBP_MenuCluster when PAGES says so; its own BACK and the panel it was drawn on go, the page frame replacing
  them. WBP_ListPanel loses its panel frame.
- WBP_MainMenuWidget / WBP_PauseMenu: TopLevel groups what shows while no page is open; the main menu's wears
  WBP_MenuFrame (TopFrame) in place of its own screen frame, so it shares the pages' window; every page is a direct child
  filling the screen, collapsed. The pause menu holds the settings pages WBP_Settings held.

Never overwrites a hand edit: the frames are built only when created, a page is converted once (it then holds
PageFrame) and only when the widgets it loses carry no hand edit, and every value goes through asset_guard.write
(kept values and skipped pages are listed in AI/Output/menu_pages.txt).
Usage: run via MCP execute_script, after the build that adds UGeoMenuRootWidget, after import_icons.py and every
other page script.
"""
import importlib
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
# rail_style first: wings_hud reloads asset_guard last, so every write below shares one ledger state.
import rail_style
import wings_hud

rail_style = importlib.reload(rail_style)
wings_hud = importlib.reload(wings_hud)
asset_guard = wings_hud.asset_guard
add, write_slot = wings_hud.add, wings_hud.write_slot

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
HUD = "/Game/HUD"
MENU_FRAME_PATH = f"{HUD}/WBP_MenuFrame"
PAGE_FRAME_PATH = f"{HUD}/WBP_MenuPageFrame"
CLUSTER_PATH = f"{HUD}/WBP_MenuCluster"
LIST_PANEL_PATH = f"{HUD}/WBP_ListPanel"
MAIN_MENU_PATH = f"{HUD}/MainMenu/WBP_MainMenuWidget"
PAUSE_PATH = f"{HUD}/InGameMenu/WBP_PauseMenu"
SETTINGS_PATH = f"{HUD}/InGameMenu/WBP_Settings"
BUTTON_CLASS = f"{HUD}/WBP_GeoButton.WBP_GeoButton_C"
CLOSE_ICON_PATH = f"{HUD}/Icons/DA_Icon_Menu_Remove"

H = unreal.HorizontalAlignment
V = unreal.VerticalAlignment
FILL = unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)
# The cross sits inside the top padding and clear of the frame's cut corner, so it covers neither the page nor the line;
# BACK likewise in the bottom-left corner, the bottom padding deep enough to hold it.
FRAME_PADDING = unreal.Margin(48, 48, 48, 68)
EARLIER_FRAME_PADDING = [unreal.Margin(48, 48, 48, 44)]
CORNER_INSET = unreal.Margin(0, 14, 14, 0)
BACK_CORNER_INSET = unreal.Margin(12, 0, 0, 12)
CLOSE_SIZE = 32.0
CLOSE_ICON_SIZE = 14.0
# The cross's line, dim at rest and white on hover, as the design's.
CLOSE_FOREGROUND = wings_hud.ui_theme.srgb("D9CCF5")
CLOSE_HOVERED_FOREGROUND = wings_hud.ui_theme.srgb("FFFFFF")
CLUSTER_WIDTH = 720.0
CLUSTER_PADDING = unreal.Margin(32, 28, 32, 28)
NO_PADDING = unreal.Margin(0, 0, 0, 0)
BACK_WIDTH = 200.0

# Each page: its asset, the widget holding its own content, how that content sits in the window, and what the page
# frame replaces.
PAGES = [
    (f"{HUD}/MainMenu/WBP_CreateServerWidget", "VerticalBox_77", (H.H_ALIGN_CENTER, V.V_ALIGN_TOP),
     ["BackButton", "Spacer_6"]),
    (f"{HUD}/MainMenu/WBP_LocalConnect", "MenuWidth", (H.H_ALIGN_CENTER, V.V_ALIGN_CENTER), ["BackButton"]),
    (f"{HUD}/MainMenu/WBP_BrowseServers", "ListFrame", (H.H_ALIGN_FILL, V.V_ALIGN_FILL), ["BackButton"]),
    (f"{HUD}/MainMenu/WBP_Leaderboard", "ListFrame", (H.H_ALIGN_FILL, V.V_ALIGN_FILL), ["BackButton"]),
    (SETTINGS_PATH, "Root", (H.H_ALIGN_LEFT, V.V_ALIGN_TOP),
     ["BackButton", "SoundWidget", "KeyBindingsWidget", "InterfaceWidget"]),
    (f"{HUD}/InGameMenu/WBP_SoundSettings", "Root", (H.H_ALIGN_FILL, V.V_ALIGN_TOP), ["BackButton"]),
    (f"{HUD}/InGameMenu/WBP_KeyBindings", "Root", (H.H_ALIGN_FILL, V.V_ALIGN_FILL), ["BackButton"]),
    (f"{HUD}/InGameMenu/WBP_InterfaceSettings", "Root", (H.H_ALIGN_CENTER, V.V_ALIGN_TOP),
     ["BackButton", "PanelFrame"]),
    (f"{HUD}/InGameMenu/WBP_CharacterSheet", "SheetBody", (H.H_ALIGN_FILL, V.V_ALIGN_FILL), ["BackButton"]),
    (f"{HUD}/InGameMenu/WBP_Gems", "GemsBody", (H.H_ALIGN_FILL, V.V_ALIGN_FILL), ["BackButton"]),
]

# The settings pages WBP_Settings held, now pages of the pause menu itself.
SETTINGS_PAGES = {"SoundWidget": f"{HUD}/InGameMenu/WBP_SoundSettings",
                  "KeyBindingsWidget": f"{HUD}/InGameMenu/WBP_KeyBindings",
                  "InterfaceWidget": f"{HUD}/InGameMenu/WBP_InterfaceSettings"}
PAUSE_PAGES = ["SettingsWidget", "LeaderboardWidget", "CharacterWidget", "GemsWidget",
               *SETTINGS_PAGES]
MAIN_MENU_PAGES = ["CreateServerWidget", "BrowseServerWidget", "LocalConnectWidget", "LeaderboardWidget"]
MAIN_MENU_TOP_LEVEL = ["MenuDecor", "VB_MainButtons"]

# The compact pages, too short for the window: each sits in a cluster. Page asset -> the widget holding its content.
CLUSTERED_PAGES = {f"{HUD}/MainMenu/WBP_CreateServerWidget": "VerticalBox_77",
                   f"{HUD}/MainMenu/WBP_LocalConnect": "MenuWidth",
                   SETTINGS_PATH: "Root",
                   f"{HUD}/InGameMenu/WBP_SoundSettings": "Root",
                   f"{HUD}/InGameMenu/WBP_InterfaceSettings": "Root"}

skipped = []


def build_menu_frame():
    """Layer 1, the bare window."""
    path = MENU_FRAME_PATH
    wbp = wings_hud.create_widget(path, "GeoMenuFrameWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.Overlay, "Root")
        add(wbp, path, unreal.SizeBox, "FrameBox", "Root")
        add(wbp, path, unreal.Overlay, "FrameOverlay", "FrameBox")
        add(wbp, path, unreal.GeoFrame, "PanelFrame", "FrameOverlay")
        add(wbp, path, unreal.NamedSlot, "ContentSlot", "PanelFrame")
        add(wbp, path, unreal.NamedSlot, "CornerSlot", "FrameOverlay")
    wbp = unreal.load_asset(path)
    add(wbp, path, unreal.NamedSlot, "BackCornerSlot", "FrameOverlay")
    # Fills the screen; the C++ insets it by the theme's margin. It was centred at a fixed size before.
    box_slot = UTIL.find_widget(wbp, "FrameBox").get_editor_property("slot")
    asset_guard.write(path, box_slot, "horizontal_alignment", H.H_ALIGN_FILL, earlier=[H.H_ALIGN_CENTER])
    asset_guard.write(path, box_slot, "vertical_alignment", V.V_ALIGN_FILL, earlier=[V.V_ALIGN_CENTER])
    panel = UTIL.find_widget(wbp, "PanelFrame")
    asset_guard.write(path, panel, "frame_style", wings_hud.frame("DA_Frame_Panel"))
    asset_guard.write(path, panel, "padding", FRAME_PADDING, earlier=EARLIER_FRAME_PADDING)
    write_slot(path, panel.get_editor_property("slot"),
               {"horizontal_alignment": H.H_ALIGN_FILL, "vertical_alignment": V.V_ALIGN_FILL})
    write_slot(path, UTIL.find_widget(wbp, "CornerSlot").get_editor_property("slot"),
               {"horizontal_alignment": H.H_ALIGN_RIGHT, "vertical_alignment": V.V_ALIGN_TOP, "padding": CORNER_INSET})
    write_slot(path, UTIL.find_widget(wbp, "BackCornerSlot").get_editor_property("slot"),
               {"horizontal_alignment": H.H_ALIGN_LEFT, "vertical_alignment": V.V_ALIGN_BOTTOM,
                "padding": BACK_CORNER_INSET})
    wings_hud.finish(wbp)
    return wbp


def build_page_frame(menu_frame):
    """Layer 2: the window with BACK and the close cross."""
    path = PAGE_FRAME_PATH
    wbp = wings_hud.create_widget(path, "GeoMenuPageFrameWidget")
    close_icon = unreal.load_asset(CLOSE_ICON_PATH)
    if not close_icon:
        raise RuntimeError(f"{CLOSE_ICON_PATH} is missing: run import_icons.py first")
    if wbp:
        UTIL.construct_widget_in_tree(wbp, menu_frame.generated_class(), "Frame", True)
        UTIL.set_root_widget(wbp, "Frame")
        UTIL.construct_widget_in_tree(wbp, unreal.VerticalBox, "PageBox", True)
        UTIL.set_named_slot_content(wbp, "Frame", "ContentSlot", "PageBox")
        _, page_slot = add(wbp, path, unreal.NamedSlot, "PageSlot", "PageBox")
        write_slot(path, page_slot, {"size": FILL})
        back_box = UTIL.construct_widget_in_tree(wbp, unreal.SizeBox, "BackBox", True)
        for key, value in {"override_min_desired_width": True, "min_desired_width": BACK_WIDTH}.items():
            asset_guard.write(path, back_box, key, value)
        add(wbp, path, unreal.load_class(None, BUTTON_CLASS), "BackButton", "BackBox", {"label": unreal.Text("BACK")})

        UTIL.construct_widget_in_tree(wbp, unreal.SizeBox, "CloseBox", True)
        UTIL.set_named_slot_content(wbp, "Frame", "CornerSlot", "CloseBox")
    wbp = unreal.load_asset(path)
    # A Rail button like BACK: the button frame, lit on hover and focus, round the icon. A SizeBox holds one child, so
    # the cross is rebuilt from the top: constructing the button first takes the old one out of CloseBox.
    close_frame = UTIL.find_widget(wbp, "CloseFrame")
    if not close_frame or not close_frame.get_parent():
        UTIL.construct_widget_in_tree(wbp, unreal.GeoButton, "CloseButton", True)
        UTIL.construct_widget_in_tree(wbp, unreal.GeoFrame, "CloseFrame", True)
        UTIL.attach_widget(wbp, "CloseBox", "CloseFrame")
        UTIL.attach_widget(wbp, "CloseFrame", "CloseButton")
        UTIL.construct_widget_in_tree(wbp, unreal.GeoIconImage, "CloseIcon", True)
        UTIL.attach_widget(wbp, "CloseButton", "CloseIcon")
        # Fresh widgets: what the ledger recorded for their predecessors no longer applies.
        fresh = {"CloseIcon": {"icon": close_icon, "size": CLOSE_ICON_SIZE, "tint_with_foreground": True},
                 "CloseFrame": {"frame_style": wings_hud.frame("DA_Frame_Button"),
                                "activate_on_hover_and_focus": True, "padding": NO_PADDING}}
        for name, values in fresh.items():
            for key, value in values.items():
                UTIL.find_widget(wbp, name).set_editor_property(key, value)
        UTIL.find_widget(wbp, "CloseIcon").get_editor_property("slot").set_editor_property("padding", NO_PADDING)
    close_box = UTIL.find_widget(wbp, "CloseBox")
    for key, value in {"override_width_override": True, "width_override": CLOSE_SIZE,
                       "override_height_override": True, "height_override": CLOSE_SIZE}.items():
        asset_guard.write(path, close_box, key, value, earlier=[36.0, 40.0])
    close_frame = UTIL.find_widget(wbp, "CloseFrame")
    for key, value in {"frame_style": wings_hud.frame("DA_Frame_Button"), "activate_on_hover_and_focus": True,
                       "padding": NO_PADDING}.items():
        asset_guard.write(path, close_frame, key, value)
    # The frame is the cross's look: the button draws nothing, its foreground tints the icon.
    close_button = UTIL.find_widget(wbp, "CloseButton")
    style = rail_style.cleared_style(close_button.get_editor_property("widget_style").copy())
    style.set_editor_property("normal_foreground", unreal.SlateColor(specified_color=CLOSE_FOREGROUND))
    for state in ["hovered_foreground", "pressed_foreground"]:
        style.set_editor_property(state, unreal.SlateColor(specified_color=CLOSE_HOVERED_FOREGROUND))
    asset_guard.write(path, close_button, "widget_style", style)
    icon = UTIL.find_widget(wbp, "CloseIcon")
    asset_guard.write(path, icon, "size", CLOSE_ICON_SIZE, earlier=[16.0, 18.0])
    asset_guard.write(path, icon, "tint_with_foreground", True)
    write_slot(path, icon.get_editor_property("slot"),
               {"padding": NO_PADDING, "horizontal_alignment": H.H_ALIGN_CENTER, "vertical_alignment": V.V_ALIGN_CENTER})
    # BACK in the window's bottom-left corner, over the bottom padding, so the page keeps the whole height. It was under
    # the page before; a named slot's content has no parent.
    if UTIL.find_widget(wbp, "BackBox").get_parent():
        UTIL.set_named_slot_content(wbp, "Frame", "BackCornerSlot", "BackBox")
    wings_hud.finish(wbp)
    return wbp


def build_cluster():
    """A section of the window at one width, holding a compact page."""
    path = CLUSTER_PATH
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        asset_guard.created(path)
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", unreal.UserWidget)
        folder, name = path.rsplit("/", 1)
        wbp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.WidgetBlueprint, factory)
        UTIL.construct_widget_in_tree(wbp, unreal.SizeBox, "ClusterBox", True)
        UTIL.set_root_widget(wbp, "ClusterBox")
        add(wbp, path, unreal.GeoFrame, "ClusterFrame", "ClusterBox")
        add(wbp, path, unreal.NamedSlot, "ClusterSlot", "ClusterFrame")
    wbp = unreal.load_asset(path)
    box = UTIL.find_widget(wbp, "ClusterBox")
    asset_guard.write(path, box, "override_width_override", True)
    asset_guard.write(path, box, "width_override", CLUSTER_WIDTH)
    cluster_frame = UTIL.find_widget(wbp, "ClusterFrame")
    asset_guard.write(path, cluster_frame, "frame_style", wings_hud.frame("DA_Frame_Section"))
    asset_guard.write(path, cluster_frame, "padding", CLUSTER_PADDING)
    write_slot(path, UTIL.find_widget(wbp, "ClusterSlot").get_editor_property("slot"),
               {"horizontal_alignment": H.H_ALIGN_FILL, "vertical_alignment": V.V_ALIGN_FILL})
    wings_hud.finish(wbp)
    return wbp


def wear_cluster(cluster, path, content):
    """Content moves from the page's PageContent into a cluster centred there. Once per page."""
    wbp = unreal.load_asset(path)
    if UTIL.find_widget(wbp, "Cluster") or not UTIL.find_widget(wbp, "PageFrame"):
        return
    UTIL.construct_widget_in_tree(wbp, cluster.generated_class(), "Cluster", True)
    cluster_slot = UTIL.attach_widget(wbp, "PageContent", "Cluster")
    cluster_slot.set_editor_property("horizontal_alignment", H.H_ALIGN_CENTER)
    cluster_slot.set_editor_property("vertical_alignment", V.V_ALIGN_CENTER)
    UTIL.set_named_slot_content(wbp, "Cluster", "ClusterSlot", content)
    wings_hud.finish(wbp)


def hand_edited(path, names):
    """The hand edits on the widgets names of path, which removing them would lose."""
    return [edit for edit in asset_guard.hand_edits(path) if any(f".{name}:" in edit for name in names)]


def wear_page_frame(page_frame, path, content, alignment, replaced):
    """The page's root becomes PageFrame with content in its PageSlot; the widgets it replaces go. Once per page."""
    wbp = unreal.load_asset(path)
    if UTIL.find_widget(wbp, "PageFrame"):
        return
    replaced = [name for name in replaced if UTIL.find_widget(wbp, name)]
    # The panels content sat in go with the old root.
    dropped = []
    ancestor = UTIL.find_widget(wbp, content).get_parent()
    while ancestor:
        dropped.append(ancestor.get_name())
        ancestor = ancestor.get_parent()
    if hand_edited(path, replaced + dropped):
        skipped.append(f"{path}: hand edits on {replaced + dropped}, fold them into the page frame first")
        return

    UTIL.construct_widget_in_tree(wbp, page_frame.generated_class(), "PageFrame", True)
    UTIL.construct_widget_in_tree(wbp, unreal.Overlay, "PageContent", True)
    UTIL.set_named_slot_content(wbp, "PageFrame", "PageSlot", "PageContent")
    content_slot = UTIL.attach_widget(wbp, "PageContent", content)
    content_slot.set_editor_property("horizontal_alignment", alignment[0])
    content_slot.set_editor_property("vertical_alignment", alignment[1])
    for name in replaced:
        UTIL.remove_widget(wbp, name)
    UTIL.set_root_widget(wbp, "PageFrame")
    wings_hud.finish(wbp)


def strip_list_frame():
    """The list sits inside the page frame, which draws the panel."""
    wbp = unreal.load_asset(LIST_PANEL_PATH)
    if UTIL.find_widget(wbp, "PanelFrame"):
        if hand_edited(LIST_PANEL_PATH, ["PanelFrame"]):
            skipped.append(f"{LIST_PANEL_PATH}: hand edits on PanelFrame")
        else:
            UTIL.remove_widget(wbp, "PanelFrame")
            wings_hud.finish(wbp)


def group_top_level(wbp, root_panel, top_level_class, children):
    """Puts children under a new TopLevel at the place of the first, each keeping its slot layout."""
    if UTIL.find_widget(wbp, "TopLevel"):
        return
    first = UTIL.find_widget(wbp, children[0])
    index = first.get_parent().get_child_index(first)
    slot_values = {name: rail_style.read_properties(UTIL.find_widget(wbp, name).get_editor_property("slot"),
                                                    rail_style.CARRIED_SLOT_PROPERTIES) for name in children}
    UTIL.construct_widget_in_tree(wbp, top_level_class, "TopLevel", True)
    fill(UTIL.attach_widget(wbp, root_panel, "TopLevel", index))
    for name in children:
        rail_style.write_properties(UTIL.attach_widget(wbp, "TopLevel", name), slot_values[name])


def fill(slot):
    """A slot whose widget covers the whole panel."""
    if isinstance(slot, unreal.CanvasPanelSlot):
        slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0, 0), maximum=unreal.Vector2D(1, 1)))
        slot.set_alignment(unreal.Vector2D(0, 0))
        slot.set_offsets(unreal.Margin(0, 0, 0, 0))
        slot.set_auto_size(False)
    else:
        slot.set_editor_property("horizontal_alignment", H.H_ALIGN_FILL)
        slot.set_editor_property("vertical_alignment", V.V_ALIGN_FILL)
        slot.set_editor_property("padding", unreal.Margin(0, 0, 0, 0))


def anchor_data(offsets, anchors, alignment):
    return unreal.AnchorData(offsets=unreal.Margin(*offsets),
                             anchors=unreal.Anchors(minimum=unreal.Vector2D(*anchors[:2]),
                                                    maximum=unreal.Vector2D(*anchors[2:])),
                             alignment=unreal.Vector2D(*alignment))


# How the main menu placed its panels before they shared one window: centred at their own size, or inset by the list
# margin rail_sub_panels.py wrote.
EARLIER_PAGE_LAYOUTS = [anchor_data((0, 0, 523, 507), (.5, .5, .5, .5), (.5, .5)),
                        anchor_data((0, 0, 100, 30), (.5, .5, .5, .5), (.5, .5)),
                        anchor_data((64, 64, 64, 64), (0, 0, 1, 1), (0, 0))]


def place_pages(wbp, path, pages):
    """Every page fills the screen, collapsed until the menu opens it."""
    for name in pages:
        page = UTIL.find_widget(wbp, name)
        slot = page.get_editor_property("slot")
        if isinstance(slot, unreal.CanvasPanelSlot):
            asset_guard.write(path, slot, "layout_data", anchor_data((0, 0, 0, 0), (0, 0, 1, 1), (0, 0)),
                              earlier=EARLIER_PAGE_LAYOUTS)
            asset_guard.write(path, slot, "auto_size", False, earlier=[True])
        else:
            write_slot(path, slot, {"horizontal_alignment": H.H_ALIGN_FILL, "vertical_alignment": V.V_ALIGN_FILL,
                                    "padding": unreal.Margin(0, 0, 0, 0)})
        asset_guard.write(path, page, "visibility", unreal.SlateVisibility.COLLAPSED)


def share_menu_frame(wbp, menu_frame):
    """The top level's own screen frame gives way to the window every page wears, so the frame never changes when a page
    opens."""
    if UTIL.find_widget(wbp, "TopFrame") or not UTIL.find_widget(wbp, "ScreenFrame"):
        return
    if hand_edited(MAIN_MENU_PATH, ["ScreenFrame"]):
        skipped.append(f"{MAIN_MENU_PATH}: hand edits on ScreenFrame, fold them into WBP_MenuFrame first")
        return
    UTIL.construct_widget_in_tree(wbp, menu_frame.generated_class(), "TopFrame", True)
    fill(UTIL.attach_widget(wbp, "MenuDecor", "TopFrame", 0))
    UTIL.remove_widget(wbp, "ScreenFrame")


def build_main_menu(menu_frame):
    wbp = unreal.load_asset(MAIN_MENU_PATH)
    group_top_level(wbp, "CanvasPanel_0", unreal.CanvasPanel, MAIN_MENU_TOP_LEVEL)
    share_menu_frame(wbp, menu_frame)
    place_pages(wbp, MAIN_MENU_PATH, MAIN_MENU_PAGES)
    wings_hud.finish(wbp)


def build_pause_menu():
    wbp = unreal.load_asset(PAUSE_PATH)
    group_top_level(wbp, "MenuOverlayRoot", unreal.Overlay, ["MenuSizeBox"])
    if UTIL.find_widget(wbp, "SettingsWidget").get_parent().get_name() != "MenuOverlayRoot":
        UTIL.attach_widget(wbp, "MenuOverlayRoot", "SettingsWidget")
    for name, page_path in SETTINGS_PAGES.items():
        if not UTIL.find_widget(wbp, name):
            UTIL.construct_widget_in_tree(wbp, unreal.load_asset(page_path).generated_class(), name, True)
            UTIL.attach_widget(wbp, "MenuOverlayRoot", name)
    place_pages(wbp, PAUSE_PATH, [name for name in PAUSE_PAGES if UTIL.find_widget(wbp, name)])
    wings_hud.finish(wbp)


def run():
    menu_frame = build_menu_frame()
    page_frame = build_page_frame(menu_frame)
    for path, content, alignment, replaced in PAGES:
        wear_page_frame(page_frame, path, content, alignment, replaced)
    cluster = build_cluster()
    for path, content in CLUSTERED_PAGES.items():
        wear_cluster(cluster, path, content)
    strip_list_frame()
    build_main_menu(menu_frame)
    build_pause_menu()
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "menu_pages.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["skipped: " + line for line in skipped]
                                                        + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
