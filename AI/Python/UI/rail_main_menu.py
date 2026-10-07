"""
Lay the main menu out in the Rail look: the screen framed by the arena-sized rail, a hex shell turning on the right,
the title over the three class shapes, the button column on the left, the player top right, key hints bottom left.
Every look comes from generic widgets (UGeoFrame, UGeoShape, UGeoText) and the style assets; this only places them.
Everything but the button column and the player sits in "MenuDecor", which the menu hides while a sub-panel is open.
Usage: run via MCP execute_script, after ui_theme.py and rail_style.py. Re-run-safe: every added widget is rebuilt.
"""
import unreal

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
RAIL_STYLE_PATH = unreal.Paths.project_dir() + "AI/Python/UI/rail_style.py"
RAIL_STYLE = {}
exec(compile(open(RAIL_STYLE_PATH, encoding="utf-8").read(), RAIL_STYLE_PATH, "exec"), RAIL_STYLE)
MAIN_MENU_PATH = "/Game/HUD/MainMenu/WBP_MainMenuWidget"
CANVAS = "CanvasPanel_0"
STYLE_DIR = "/Game/HUD/Style"

# Layout in 1920x1080 canvas units.
SCREEN_MARGIN = 38
LEFT = 154
TITLE_TOP = 140
BUTTONS_TOP = 440
BUTTON_WIDTH = 480
BUTTON_GAP = 22
QUIT_EXTRA_GAP = 22
HEX_CENTRE = (0.7625, 0.478)
HEX_SIZE = 624
HINTS_BOTTOM = 92


def srgb(hex_code, alpha=1.0):
    hex_code = hex_code.lstrip("#")
    color = unreal.Color(r=int(hex_code[0:2], 16), g=int(hex_code[2:4], 16), b=int(hex_code[4:6], 16), a=255)
    linear = unreal.MathLibrary.conv_color_to_linear_color(color)
    linear.a = alpha
    return linear


CLASS_SHAPES = [  # name, sides, rotation, colour — the class colours of the game
    ("SquareShape", 4, 45.0, srgb("7CBCFF")),
    ("CircleShape", 0, 0.0, srgb("7CF3AA")),
    ("TriangleShape", 3, 0.0, srgb("FFE759")),
]

HEX_LAYERS = [  # name, size, line, colour, spin (deg/s), filled, outline style (replaces line, colour and fill)
    ("HexOuter", HEX_SIZE, 2.4, srgb("F4EEFF"), 2.6, False, "DA_Frame_Ornament"),
    ("HexMiddle", 408, 1.8, srgb("B37CFF", .7), -4.0, False, None),
    ("HexInner", 240, 1.2, srgb("F4EEFF", .5), -4.0, False, None),
    ("HexCore", 86, 1.0, srgb("B37CFF", .9), 0.0, True, None),
]


def canvas_place(widget, anchor_min, anchor_max, alignment, offsets, auto_size=False):
    slot = widget.get_editor_property("slot")
    slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(*anchor_min), maximum=unreal.Vector2D(*anchor_max)))
    slot.set_alignment(unreal.Vector2D(*alignment))
    slot.set_offsets(unreal.Margin(*offsets))
    slot.set_auto_size(auto_size)
    return slot


def add(wbp, widget_class, name, parent, index=-1):
    widget = UTIL.construct_widget_in_tree(wbp, widget_class, name, True)
    slot = UTIL.attach_widget(wbp, parent, name, index)
    return widget, slot


def shape(widget, sides, size, rotation, color, line=1.5, spin=0.0, filled=False):
    widget.set_editor_property("sides", sides)
    widget.set_editor_property("size", size)
    widget.set_editor_property("rotation", rotation)
    widget.set_editor_property("color", color)
    widget.set_editor_property("line_thickness", line)
    widget.set_editor_property("spin_speed", spin)
    widget.set_editor_property("filled", filled)


def text(widget, role, label):
    widget.set_editor_property("role", role)
    widget.set_editor_property("text", unreal.Text(label))


def build_screen_frame(wbp):
    """The arena-sized rail framing the whole screen, behind the top level and every sub-panel alike."""
    frame, _ = add(wbp, unreal.GeoFrame, "ScreenFrame", CANVAS, 0)
    frame.set_editor_property("frame_style", unreal.load_asset(f"{STYLE_DIR}/DA_Frame_Screen"))
    frame.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    canvas_place(frame, (0, 0), (1, 1), (0, 0), (SCREEN_MARGIN,) * 4)


def build_decor(wbp):
    """MenuDecor: a full-screen canvas over the screen frame holding the hex shell, the title and the hints."""
    decor, slot = add(wbp, unreal.CanvasPanel, "MenuDecor", CANVAS, 1)
    decor.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    canvas_place(decor, (0, 0), (1, 1), (0, 0), (0, 0, 0, 0))


    hex_shell, _ = add(wbp, unreal.Overlay, "HexShell", "MenuDecor")
    canvas_place(hex_shell, HEX_CENTRE, HEX_CENTRE, (.5, .5), (0, 0, HEX_SIZE, HEX_SIZE))
    for name, size, line, color, spin, filled, outline_style in HEX_LAYERS:
        layer, layer_slot = add(wbp, unreal.GeoShape, name, "HexShell")
        shape(layer, 6, size, 90.0, color, line, spin, filled)
        if outline_style:
            layer.set_editor_property("outline_style", unreal.load_asset(f"{STYLE_DIR}/{outline_style}"))
        layer_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
        layer_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)

    title_box, _ = add(wbp, unreal.VerticalBox, "TitleBox", "MenuDecor")
    canvas_place(title_box, (0, 0), (0, 0), (0, 0), (LEFT, TITLE_TOP, 0, 0), auto_size=True)
    shapes_row, shapes_slot = add(wbp, unreal.HorizontalBox, "ClassShapes", "TitleBox")
    shapes_slot.set_editor_property("padding", unreal.Margin(4, 0, 0, 10))
    for name, sides, rotation, color in CLASS_SHAPES:
        class_shape, class_slot = add(wbp, unreal.GeoShape, name, "ClassShapes")
        shape(class_shape, sides, 32, rotation, color, filled=True)
        class_slot.set_editor_property("padding", unreal.Margin(0, 0, 16, 0))
    title, _ = add(wbp, unreal.GeoText, "TitleText", "TitleBox")
    text(title, unreal.GeoTextRole.TITLE, "GeoTrinity")
    subtitle, subtitle_slot = add(wbp, unreal.GeoText, "SubtitleText", "TitleBox")
    text(subtitle, unreal.GeoTextRole.LABEL, "Tank · Heal · DPS — co-op boss fights")
    subtitle_slot.set_editor_property("padding", unreal.Margin(4, 6, 0, 0))

    hints, _ = add(wbp, unreal.GeoText, "HintsText", "MenuDecor")
    text(hints, unreal.GeoTextRole.LABEL, "↑↓  navigate      enter  select      esc  back")
    canvas_place(hints, (0, 1), (0, 1), (0, 1), (LEFT, -HINTS_BOTTOM, 0, 0), auto_size=True)


def place_buttons(wbp):
    box = UTIL.find_widget(wbp, "VB_MainButtons")
    buttons = box.get_all_children()
    height = 5 * 66 + 4 * BUTTON_GAP + QUIT_EXTRA_GAP
    canvas_place(box, (0, 0), (0, 0), (0, 0), (LEFT, BUTTONS_TOP, BUTTON_WIDTH, height))
    for index, button in enumerate(buttons):
        top = 0 if index == 0 else BUTTON_GAP + (QUIT_EXTRA_GAP if button.get_name() == "QuitButton" else 0)
        slot = button.get_editor_property("slot")
        slot.set_padding(unreal.Margin(0, top, 0, 0))
        slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)


def place_player(wbp):
    """Avatar and name together in the top-right corner."""
    if UTIL.find_widget(wbp, "PlayerBox"):
        for name in ["SteamAvatar", "Text_PlayerName"]:
            UTIL.attach_widget(wbp, CANVAS, name)
        UTIL.remove_widget(wbp, "PlayerBox")
    player = UTIL.group_widgets_into_panel(wbp, CANVAS, "PlayerBox", unreal.HorizontalBox,
                                           ["SteamAvatar", "Text_PlayerName"], unreal.Margin(0, 0, 0, 0))
    canvas_place(player, (1, 0), (1, 0), (1, 0), (-LEFT, 104, 0, 0), auto_size=True)
    name = RAIL_STYLE["swap_widget_class"](wbp, "Text_PlayerName", unreal.GeoText)
    name.set_editor_property("role", unreal.GeoTextRole.MONO)
    avatar_slot = UTIL.find_widget(wbp, "SteamAvatar").get_editor_property("slot")
    avatar_slot.set_editor_property("padding", unreal.Margin(0, 0, 14, 0))
    avatar_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    name_slot = UTIL.find_widget(wbp, "Text_PlayerName").get_editor_property("slot")
    name_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
    UTIL.find_widget(wbp, "SteamAvatar").set_desired_size_override(unreal.Vector2D(44, 44))


def build_main_menu():
    wbp = unreal.load_asset(MAIN_MENU_PATH)
    if UTIL.find_widget(wbp, "PlayerBox") and UTIL.find_widget(wbp, "PlayerBox").get_parent().get_name() == "MenuDecor":
        UTIL.attach_widget(wbp, CANVAS, "PlayerBox")
    for name in ["MenuDecor", "ScreenFrame"]:
        if UTIL.find_widget(wbp, name):
            UTIL.remove_widget(wbp, name)
    build_screen_frame(wbp)
    build_decor(wbp)
    place_buttons(wbp)
    place_player(wbp)
    UTIL.commit_tree(wbp)
    UTIL.inspect_widget_blueprint(RAIL_STYLE["reload_blueprint"](wbp))


if __name__ == "__main__":
    build_main_menu()
