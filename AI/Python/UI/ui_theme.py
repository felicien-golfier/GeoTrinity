"""
Create the menu theme: DA_UITheme (UGeoUITheme) and one UGeoFrameStyle per kind of frame, in /Game/HUD/Style, and name
the theme in Project Settings > Geo UI. Values are seeded only when an asset is created, so re-running never resets
what was tuned in the editor; pass reseed=True to rewrite every seeded value.
Look: flat geometry on near-black, a near-white line with a violet glow, small squares travelling the outlines.
Usage: run via MCP execute_script, after import_fonts.py.
"""
import os
import unreal

STYLE_DIR = "/Game/HUD/Style"
FONT_DIR = "/Game/HUD/Assets/Fonts"
THEME_PATH = STYLE_DIR + "/DA_UITheme"
CONFIG = os.path.join(unreal.Paths.project_config_dir(), "DefaultGame.ini")
SETTINGS_SECTION = "[/Script/GeoTrinityUI.GeoUISettings]"


def srgb(hex_code, alpha=1.0):
    """Linear colour from an sRGB hex code, the way a colour picker shows it."""
    hex_code = hex_code.lstrip("#")
    color = unreal.Color(r=int(hex_code[0:2], 16), g=int(hex_code[2:4], 16), b=int(hex_code[4:6], 16),
                         a=int(alpha * 255))
    linear = unreal.MathLibrary.conv_color_to_linear_color(color)
    linear.a = alpha
    return linear


PALETTE = {
    "night": srgb("04040A"),
    "panel": srgb("08070F"),
    "field": srgb("0C0A16"),
    "line": srgb("EDE4FF"),
    "violet": srgb("B37CFF"),
    "text": srgb("F4EEFF"),
    "body": srgb("D9D0F0"),
    "dim": srgb("8F86AD"),
    "faint": srgb("3A3352"),
}


def font(face, size, spacing=0):
    info = unreal.SlateFontInfo()
    info.set_editor_property("font_object", unreal.load_asset(f"{FONT_DIR}/{face}_Font"))
    info.set_editor_property("typeface_font_name", "Default")
    info.set_editor_property("size", size)
    info.set_editor_property("letter_spacing", spacing)
    return info


def text_style(face, size, color, spacing=0, upper=False):
    style = unreal.GeoTextStyle()
    style.set_editor_property("font", font(face, size, spacing))
    style.set_editor_property("color", unreal.SlateColor(specified_color=color))
    style.set_editor_property("transform",
                              unreal.TextTransformPolicy.TO_UPPER if upper else unreal.TextTransformPolicy.NONE)
    return style


def brush(color=None, size=(16, 16), outline=None, outline_width=0.0, radius=0.0):
    """Flat box brush: a fill (or none), an optional outline, square corners unless radius is set."""
    slate_brush = unreal.SlateBrush()
    image_size = unreal.DeprecateSlateVector2D()
    image_size.set_editor_property("x", size[0])
    image_size.set_editor_property("y", size[1])
    slate_brush.set_editor_property("image_size", image_size)
    if color is None and outline is None:
        slate_brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
        return slate_brush

    slate_brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.ROUNDED_BOX)
    fill = color if color is not None else unreal.LinearColor(0, 0, 0, 0)
    slate_brush.set_editor_property("tint_color", unreal.SlateColor(specified_color=fill))
    settings = unreal.SlateBrushOutlineSettings()
    settings.set_editor_property("corner_radii", unreal.Vector4(radius, radius, radius, radius))
    settings.set_editor_property("rounding_type", unreal.SlateBrushRoundingType.FIXED_RADIUS)
    settings.set_editor_property("width", outline_width)
    settings.set_editor_property("color", unreal.SlateColor(specified_color=outline or unreal.LinearColor(0, 0, 0, 0)))
    slate_brush.set_editor_property("outline_settings", settings)
    return slate_brush


def font_text_block_style(base, face, size, color):
    style = base.copy()
    style.set_editor_property("font", font(face, size))
    style.set_editor_property("color_and_opacity", unreal.SlateColor(specified_color=color))
    return style


# --- Frame styles: one per kind of frame ------------------------------------------------------------------------------
P = PALETTE
FRAME_STYLES = {
    # Every clickable button: dim violet line, goes white with a glow and faster runners on hover/focus.
    "DA_Frame_Button": dict(
        line_color=srgb("B37CFF", .55), active_line_color=P["line"], line_thickness=1.5, corner_cut=8.0,
        glow_color=srgb("B37CFF", 0.0), active_glow_color=srgb("B37CFF", .6), glow_thickness=12.0,
        fill_color=srgb("B37CFF", .05), active_fill_color=srgb("B37CFF", .14),
        runner_count=1, active_runner_count=2, runner_size=6.0, runner_speed=28.0, active_runner_speed=170.0,
        runner_roll=1.0, alternate_hollow_runners=True, runner_color=P["line"], hollow_runner_thickness=1.5,
        activation_seconds=.12),
    # Windows: a dark plate with a steady violet line and two slow runners.
    "DA_Frame_Panel": dict(
        line_color=srgb("B37CFF", .6), active_line_color=srgb("B37CFF", .6), line_thickness=1.5, corner_cut=18.0,
        glow_color=srgb("B37CFF", .3), active_glow_color=srgb("B37CFF", .3), glow_thickness=16.0,
        fill_color=srgb("08070F", .94), active_fill_color=srgb("08070F", .94),
        runner_count=2, active_runner_count=2, runner_size=7.0, runner_speed=36.0, active_runner_speed=36.0,
        runner_roll=1.0, alternate_hollow_runners=True, runner_color=P["line"], hollow_runner_thickness=1.5),
    # List rows: invisible until hovered or selected, then a white line with two runners.
    "DA_Frame_Row": dict(
        line_color=srgb("EDE4FF", 0.0), active_line_color=srgb("EDE4FF", .9), line_thickness=1.5, corner_cut=6.0,
        glow_color=srgb("B37CFF", 0.0), active_glow_color=srgb("B37CFF", .22), glow_thickness=6.0,
        fill_color=srgb("B37CFF", 0.0), active_fill_color=srgb("B37CFF", .12),
        runner_count=0, active_runner_count=2, runner_size=5.0, runner_speed=0.0, active_runner_speed=140.0,
        runner_roll=1.0, alternate_hollow_runners=True, runner_color=P["line"], hollow_runner_thickness=1.2,
        activation_seconds=.1),
    # Text fields, drop-downs: a dark well, white line and one runner while focused.
    "DA_Frame_Field": dict(
        line_color=srgb("8F86AD", .7), active_line_color=P["line"], line_thickness=1.2, corner_cut=6.0,
        glow_color=srgb("B37CFF", 0.0), active_glow_color=srgb("B37CFF", .25), glow_thickness=6.0,
        fill_color=srgb("0C0A16", .95), active_fill_color=srgb("100D1E", .95),
        runner_count=0, active_runner_count=1, runner_size=4.0, runner_speed=0.0, active_runner_speed=110.0,
        runner_roll=1.0, alternate_hollow_runners=False, runner_color=P["line"], hollow_runner_thickness=1.2,
        activation_seconds=.1),
    # Ornaments (the main menu's hex shell): a bright line in a wide violet glow, three runners going round.
    "DA_Frame_Ornament": dict(
        line_color=P["text"], active_line_color=P["text"], line_thickness=2.4, corner_cut=0.0,
        glow_color=srgb("B37CFF", .5), active_glow_color=srgb("B37CFF", .5), glow_thickness=18.0,
        fill_color=srgb("B37CFF", 0.0), active_fill_color=srgb("B37CFF", 0.0),
        runner_count=3, active_runner_count=3, runner_size=10.0, runner_speed=46.0, active_runner_speed=46.0,
        runner_roll=1.0, alternate_hollow_runners=True, runner_color=P["text"], hollow_runner_thickness=2.0),
    # Dims the fight behind an in-game menu: a flat night wash, no line.
    "DA_Frame_Scrim": dict(
        line_color=srgb("000000", 0.0), active_line_color=srgb("000000", 0.0), line_thickness=0.0, corner_cut=0.0,
        glow_color=srgb("000000", 0.0), active_glow_color=srgb("000000", 0.0), glow_thickness=0.0,
        fill_color=srgb("04040A", .72), active_fill_color=srgb("04040A", .72),
        runner_count=0, active_runner_count=0),
    # The whole screen behind a menu: night background, faint grid, the arena-sized rail with four runners.
    "DA_Frame_Screen": dict(
        line_color=srgb("B37CFF", .55), active_line_color=srgb("B37CFF", .55), line_thickness=2.0, corner_cut=28.0,
        glow_color=srgb("B37CFF", .3), active_glow_color=srgb("B37CFF", .3), glow_thickness=20.0,
        fill_color=srgb("04040A", 1.0), active_fill_color=srgb("04040A", 1.0),
        grid_spacing=48.0, grid_color=srgb("FFFFFF", .025),
        runner_count=4, active_runner_count=4, runner_size=8.0, runner_speed=55.0, active_runner_speed=55.0,
        runner_roll=1.0, alternate_hollow_runners=True, runner_color=P["line"], hollow_runner_thickness=1.5),
}


def theme_values():
    text_styles = {
        unreal.GeoTextRole.TITLE: text_style("ChakraPetch-Bold", 64, P["text"], 300, upper=True),
        unreal.GeoTextRole.HEADING: text_style("ChakraPetch-SemiBold", 30, P["text"], 200, upper=True),
        unreal.GeoTextRole.BODY: text_style("ChakraPetch-Regular", 18, P["body"]),
        unreal.GeoTextRole.LABEL: text_style("IBMPlexMono-Medium", 12, P["dim"], 150, upper=True),
        unreal.GeoTextRole.MONO: text_style("IBMPlexMono-Regular", 16, P["body"]),
        unreal.GeoTextRole.BUTTON: text_style("ChakraPetch-SemiBold", 20, P["text"], 200, upper=True),
    }
    return text_styles


def engine_style(widget_class, property_name):
    """The style an engine widget is created with: the base every themed input style starts from, so whatever this
    theme does not set (arrow images, scroll bars, sounds) keeps the engine's look."""
    return unreal.get_default_object(widget_class).get_editor_property(property_name).copy()


def seed_inputs(theme):
    none = brush()

    text_box = engine_style(unreal.EditableTextBox, "widget_style")
    for name in ["background_image_normal", "background_image_hovered", "background_image_focused",
                 "background_image_read_only"]:
        text_box.set_editor_property(name, none)
    text_box.set_editor_property("padding", unreal.Margin(14, 10, 14, 10))
    text_box.set_editor_property("text_style", font_text_block_style(text_box.get_editor_property("text_style"),
                                                                     "ChakraPetch-Regular", 18, P["text"]))
    for name in ["foreground_color", "focused_foreground_color"]:
        text_box.set_editor_property(name, unreal.SlateColor(specified_color=P["text"]))
    text_box.set_editor_property("read_only_foreground_color", unreal.SlateColor(specified_color=P["dim"]))
    theme.set_editor_property("editable_text_box_style", text_box)

    combo = engine_style(unreal.ComboBoxString, "widget_style")
    combo_button = combo.get_editor_property("combo_button_style").copy()
    button = combo_button.get_editor_property("button_style").copy()
    for name in ["normal", "hovered", "pressed"]:
        button.set_editor_property(name, none)
    button.set_editor_property("disabled", none)
    combo_button.set_editor_property("button_style", button)
    arrow = combo_button.get_editor_property("down_arrow_image").copy()
    arrow.set_editor_property("tint_color", unreal.SlateColor(specified_color=P["violet"]))
    combo_button.set_editor_property("down_arrow_image", arrow)
    combo_button.set_editor_property("menu_border_brush", brush(srgb("0C0A16"), outline=srgb("B37CFF", .6),
                                                                outline_width=1.0))
    combo.set_editor_property("combo_button_style", combo_button)
    combo.set_editor_property("content_padding", unreal.Margin(14, 8, 10, 8))
    combo.set_editor_property("menu_row_padding", unreal.Margin(14, 6, 14, 6))
    theme.set_editor_property("combo_box_style", combo)

    item = engine_style(unreal.ComboBoxString, "item_style")
    for name in ["even_row_background_brush", "odd_row_background_brush", "inactive_brush"]:
        item.set_editor_property(name, brush(srgb("0C0A16")))
    for name in ["active_brush", "selector_focused_brush"]:
        item.set_editor_property(name, brush(srgb("B37CFF", .25)))
    for name in ["active_hovered_brush", "inactive_hovered_brush", "even_row_background_hovered_brush",
                 "odd_row_background_hovered_brush"]:
        item.set_editor_property(name, brush(srgb("B37CFF", .14)))
    item.set_editor_property("text_color", unreal.SlateColor(specified_color=P["body"]))
    item.set_editor_property("selected_text_color", unreal.SlateColor(specified_color=P["text"]))
    theme.set_editor_property("combo_box_item_style", item)

    check = engine_style(unreal.CheckBox, "widget_style")
    box_size = (22, 22)
    check.set_editor_property("unchecked_image", brush(srgb("0C0A16"), box_size, srgb("8F86AD"), 1.5))
    check.set_editor_property("unchecked_hovered_image", brush(srgb("100D1E"), box_size, P["line"], 1.5))
    check.set_editor_property("unchecked_pressed_image", brush(srgb("100D1E"), box_size, P["line"], 1.5))
    check.set_editor_property("checked_image", brush(P["violet"], box_size, srgb("8F86AD"), 1.5))
    check.set_editor_property("checked_hovered_image", brush(P["violet"], box_size, P["line"], 1.5))
    check.set_editor_property("checked_pressed_image", brush(P["violet"], box_size, P["line"], 1.5))
    theme.set_editor_property("check_box_style", check)

    slider = engine_style(unreal.Slider, "widget_style")
    slider.set_editor_property("normal_bar_image", brush(P["faint"], (8, 2)))
    slider.set_editor_property("hovered_bar_image", brush(srgb("4A4266"), (8, 2)))
    slider.set_editor_property("disabled_bar_image", brush(srgb("3A3352", .5), (8, 2)))
    slider.set_editor_property("normal_thumb_image", brush(P["line"], (14, 14)))
    slider.set_editor_property("hovered_thumb_image", brush(P["violet"], (14, 14), P["line"], 1.5))
    slider.set_editor_property("disabled_thumb_image", brush(P["faint"], (14, 14)))
    slider.set_editor_property("bar_thickness", 2.0)
    theme.set_editor_property("slider_style", slider)

    scroll = engine_style(unreal.ScrollBox, "widget_bar_style")
    for name in ["horizontal_background_image", "vertical_background_image", "vertical_top_slot_image",
                 "vertical_bottom_slot_image", "horizontal_top_slot_image", "horizontal_bottom_slot_image"]:
        scroll.set_editor_property(name, none)
    scroll.set_editor_property("normal_thumb_image", brush(srgb("B37CFF", .45)))
    scroll.set_editor_property("hovered_thumb_image", brush(srgb("B37CFF", .8)))
    scroll.set_editor_property("dragged_thumb_image", brush(P["line"]))
    scroll.set_editor_property("thickness", 4.0)
    theme.set_editor_property("scroll_bar_style", scroll)

    progress = engine_style(unreal.ProgressBar, "widget_style")
    progress.set_editor_property("background_image", brush(P["faint"]))
    progress.set_editor_property("fill_image", brush(P["violet"]))
    progress.set_editor_property("marquee_image", brush(P["violet"]))
    theme.set_editor_property("progress_bar_style", progress)


def create_or_load(path, asset_class):
    """The asset at path, created when missing. Second value: whether it was just created."""
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path), False
    folder, name = path.rsplit("/", 1)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, asset_class, factory), True


def name_theme_in_project_settings(theme):
    """Point Project Settings > Geo UI at the theme, in DefaultGame.ini and in this session."""
    settings_class = unreal.load_class(None, "/Script/GeoTrinityUI.GeoUISettings")
    unreal.get_default_object(settings_class).set_editor_property("theme", theme)
    line = f"Theme={theme.get_path_name()}"
    with open(CONFIG, encoding="utf-8") as handle:
        text = handle.read()
    if SETTINGS_SECTION in text:
        head, rest = text.split(SETTINGS_SECTION, 1)
        body, separator, tail = rest.partition("\n[")
        body = "\n".join(entry for entry in body.split("\n") if not entry.startswith("Theme=")).rstrip("\n")
        text = head + SETTINGS_SECTION + body + "\n" + line + "\n" + ("\n" + separator.lstrip("\n") + tail if separator else "")
    else:
        text = text.rstrip("\n") + f"\n\n{SETTINGS_SECTION}\n{line}\n"
    with open(CONFIG, "w", encoding="utf-8") as handle:
        handle.write(text)


def build_theme(reseed=False):
    frames = {}
    for name, values in FRAME_STYLES.items():
        style, created = create_or_load(f"{STYLE_DIR}/{name}", unreal.GeoFrameStyle)
        if created or reseed:
            for key, value in values.items():
                style.set_editor_property(key, value)
            unreal.EditorAssetLibrary.save_loaded_asset(style)
        frames[name] = style

    theme, created = create_or_load(THEME_PATH, unreal.GeoUITheme)
    if created or reseed:
        theme.set_editor_property("text_styles", theme_values())
        theme.set_editor_property("default_frame_style", frames["DA_Frame_Panel"])
        theme.set_editor_property("field_frame_style", frames["DA_Frame_Field"])
        seed_inputs(theme)
        unreal.EditorAssetLibrary.save_loaded_asset(theme)

    name_theme_in_project_settings(theme)
    return theme


if __name__ == "__main__":
    build_theme(reseed=globals().get("RESEED", False))
