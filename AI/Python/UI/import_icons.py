"""
Build the vector icons (UGeoIcon, /Game/HUD/Icons/DA_Icon_<Name>) from the SVGs in SourceArt/Icons, then point the
game's data at them: each player ability's AbilityIcon in DA_AbilityInfo and the Icon of every effect data whose
GameplayEffect has one (EFFECT_ICONS). WBP_AbilitySlot's Icon becomes a UGeoIconImage.

An SVG is drawn with the subset the icon sheet uses — polygon, polyline, line, rect (with a rotate transform), circle,
path (M L H V Q A Z, absolute) — and its root's data-geo-color names the EGeoColor the icon is drawn in. A shape or
group naming its own data-geo-color is drawn in that meaning instead (an ability doing several things), at most four.
Fill and stroke only say whether a shape is filled or stroked; the colour comes from the EGeoColor alone, an Override
icon taking the SVG's own colour. stroke-opacity / fill-opacity / opacity set a stroke's Opacity (faint = secondary).

Never overwrites a hand edit (asset_guard): an icon's colour, size and strokes follow the SVG while they still hold
what a script last wrote, and a value changed by hand is kept and listed in the report; an ability's or effect's
icon is only filled while empty; the slot is only touched while its Icon is not a GeoIconImage. Usage: run via MCP execute_script (or AI/Python/Runtime/run_via_bridge.py).
"""
import math
import os
import re
import sys
import xml.etree.ElementTree as ElementTree

ICON_FOLDER = "/Game/HUD/Icons"
CIRCLE_SEGMENTS = 48
CURVE_SEGMENTS = 16

ABILITY_INFO_PATH = "/Game/AbilitySystem/Data/DA_AbilityInfo"
PLAYER_ARRAYS = ["triangle_abilities", "circle_abilities", "square_abilities", "shared_abilities"]
# Ability class asset name -> icon name.
ABILITY_ICONS = {
    "GA_Triangle_AutoProjectile": "Triangle_AutoFire",
    "GA_LaunchTurret": "Triangle_DeployTurret",
    "GA_TurretRecall": "Triangle_RecallTurrets",
    "GA_Reload": "Triangle_Reload",
    "GA_Triangle_Momentum": "Triangle_Momentum",
    "GA_Circle_ChargeBeam": "Circle_ChargeBeam",
    "GA_DeployHealingZone": "Circle_DeployHealingZone",
    "GA_MoiraBeam": "Circle_MoiraBeam",
    "GA_HealsYouWhatYouHeal": "Circle_BackHeals",
    "GA_SweetSpotCharge": "Circle_SweetSpotCharge",
    "GA_Square_AutoProjectile": "Square_AutoFire",
    "GA_Square_Deployable_Mine": "Square_DeployWall",
    "GA_Square_Special_SacrificeBeam": "Square_MartyrBeam",
    "GA_Square_Special_SacrificeDetonate": "Square_MartyrsWrath",
    "GA_Square_Passive_ShieldBurst": "Square_ShieldBurst",
    "GA_DashAbility": "Dash",
}
# GameplayEffect class asset name -> status icon name; every effect data applying one of them shows it.
EFFECT_ICONS = {
    "GE_DamageReductionBuff": "Status_DamageReduction",
    "GE_DamageMultiplierBuff": "Status_DamageBoost",
    "GE_ReceivedHealMultiplierBuff": "Status_HealReceivedBoost",
    "GE_MoiraBeam_InfinitSpeedBoost": "Status_Speed",
}
ABILITY_FOLDER = "/Game/AbilitySystem/Abilities"
GEM_LOADOUT_PATH = "/Game/HUD/InGameMenu/WBP_GemLoadout"
BUILD_TAB_ICONS = {
    "rename_build_icon": "Menu_Rename",
    "remove_build_icon": "Menu_Remove",
    "add_build_icon": "Menu_AddTab",
}
ABILITY_SLOT_PATH = "/Game/HUD/AbilityBar/WBP_AbilitySlot"


def project_folder():
    """The project root, found from this file host-side and from the editor when run through the bridge, which sets no
    __file__."""
    try:
        return os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
    except NameError:
        import unreal
        return os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))


def source_folder():
    return os.path.join(project_folder(), "SourceArt", "Icons")


# ---------------------------------------------------------------------------------------------------------------------
# SVG -> strokes (plain Python, no unreal)
# ---------------------------------------------------------------------------------------------------------------------
def numbers(text):
    return [float(value) for value in re.findall(r"-?\d*\.?\d+(?:e-?\d+)?", text)]


def pairs(text):
    values = numbers(text)
    return [(values[i], values[i + 1]) for i in range(0, len(values) - 1, 2)]


def circle_points(cx, cy, r):
    return [(cx + r * math.sin(2 * math.pi * i / CIRCLE_SEGMENTS), cy - r * math.cos(2 * math.pi * i / CIRCLE_SEGMENTS))
            for i in range(CIRCLE_SEGMENTS)]


def rotate(points, transform):
    match = re.search(r"rotate\(([^)]*)\)", transform or "")
    if not match:
        return points
    values = numbers(match.group(1)) + [0, 0]
    angle, cx, cy = math.radians(values[0]), values[1], values[2]
    cos, sin = math.cos(angle), math.sin(angle)
    return [(cx + (x - cx) * cos - (y - cy) * sin, cy + (x - cx) * sin + (y - cy) * cos) for x, y in points]


def arc_points(start, rx, ry, x_rotation, large_arc, sweep, end):
    """SVG endpoint arc to points, after the centre parameterization of the SVG spec (F.6.5)."""
    (x1, y1), (x2, y2) = start, end
    phi = math.radians(x_rotation)
    cos_phi, sin_phi = math.cos(phi), math.sin(phi)
    dx, dy = (x1 - x2) / 2, (y1 - y2) / 2
    x1p, y1p = cos_phi * dx + sin_phi * dy, -sin_phi * dx + cos_phi * dy
    rx, ry = abs(rx), abs(ry)
    scale = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry)
    if scale > 1:
        rx, ry = rx * math.sqrt(scale), ry * math.sqrt(scale)
    numerator = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p
    factor = math.sqrt(max(numerator, 0) / (rx * rx * y1p * y1p + ry * ry * x1p * x1p))
    if large_arc == sweep:
        factor = -factor
    cxp, cyp = factor * rx * y1p / ry, -factor * ry * x1p / rx
    cx = cos_phi * cxp - sin_phi * cyp + (x1 + x2) / 2
    cy = sin_phi * cxp + cos_phi * cyp + (y1 + y2) / 2

    def angle(ux, uy, vx, vy):
        value = math.atan2(ux * vy - uy * vx, ux * vx + uy * vy)
        return value

    theta = angle(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry)
    delta = angle((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry)
    if not sweep and delta > 0:
        delta -= 2 * math.pi
    elif sweep and delta < 0:
        delta += 2 * math.pi
    steps = max(2, int(abs(delta) / (2 * math.pi) * CIRCLE_SEGMENTS) + 1)
    points = []
    for step in range(1, steps + 1):
        t = theta + delta * step / steps
        x, y = rx * math.cos(t), ry * math.sin(t)
        points.append((cos_phi * x - sin_phi * y + cx, sin_phi * x + cos_phi * y + cy))
    return points


def path_points(d):
    """Points of an absolute path, and whether it closes."""
    tokens = re.findall(r"[MLHVQAZmlhvqaz]|-?\d*\.?\d+(?:e-?\d+)?", d)
    points, closed, index, command = [], False, 0, None
    while index < len(tokens):
        if tokens[index].isalpha():
            command = tokens[index]
            index += 1
            if command in "Zz":
                closed = True
                continue

        def take(count):
            nonlocal index
            values = [float(tokens[index + i]) for i in range(count)]
            index += count
            return values

        current = points[-1] if points else (0.0, 0.0)
        if command in "ML":
            points.append(tuple(take(2)))
        elif command == "H":
            points.append((take(1)[0], current[1]))
        elif command == "V":
            points.append((current[0], take(1)[0]))
        elif command == "Q":
            cx, cy, x, y = take(4)
            for step in range(1, CURVE_SEGMENTS + 1):
                t = step / CURVE_SEGMENTS
                points.append(((1 - t) ** 2 * current[0] + 2 * (1 - t) * t * cx + t * t * x,
                               (1 - t) ** 2 * current[1] + 2 * (1 - t) * t * cy + t * t * y))
        elif command == "A":
            rx, ry, x_rotation, large_arc, sweep, x, y = take(7)
            points.extend(arc_points(current, rx, ry, x_rotation, bool(large_arc), bool(sweep), (x, y)))
        else:
            raise ValueError(f"path command {command} is not supported: {d}")
    return points, closed


def shape_points(element):
    """Points of one shape element and whether it closes; None for an element that is not a shape."""
    tag = element.tag.split("}")[-1]
    get = lambda name: float(element.get(name, 0))
    if tag == "polygon":
        return pairs(element.get("points")), True
    if tag == "polyline":
        return pairs(element.get("points")), False
    if tag == "line":
        return [(get("x1"), get("y1")), (get("x2"), get("y2"))], False
    if tag == "circle":
        return circle_points(get("cx"), get("cy"), get("r")), True
    if tag == "rect":
        x, y, w, h = get("x"), get("y"), get("width"), get("height")
        return rotate([(x, y), (x + w, y), (x + w, y + h), (x, y + h)], element.get("transform")), True
    if tag == "path":
        return path_points(element.get("d"))
    return None


def parse_svg(path):
    """(EGeoColor names, the root's first, the SVG's own colour as an (r, g, b) tuple or None, view size, strokes as
    dicts, each with the index of its colour name)."""
    root = ElementTree.parse(path).getroot()
    view_size = numbers(root.get("viewBox", "0 0 44 44"))[2]
    inherited = ["fill", "stroke", "stroke-width", "stroke-opacity", "fill-opacity", "stroke-dasharray",
                 "data-geo-color"]
    color_names = [root.get("data-geo-color", "Override")]
    strokes, own_color = [], None

    def walk(element, style):
        nonlocal own_color
        style = dict(style)
        for name in inherited:
            if element.get(name) is not None:
                style[name] = element.get(name)
        style["opacity"] = style.get("opacity", 1.0) * float(element.get("opacity", 1))
        shape = shape_points(element)
        if shape:
            points, closed = shape
            filled = style.get("fill", "black") != "none"
            stroked = style.get("stroke", "none") != "none"
            for paint in [style.get("stroke"), style.get("fill")]:
                if own_color is None and paint and paint.startswith("#"):
                    own_color = tuple(int(paint[i:i + 2], 16) / 255 for i in (1, 3, 5))
            opacity = float(style.get("stroke-opacity" if stroked else "fill-opacity", 1)) * style["opacity"]
            dashes = numbers(style.get("stroke-dasharray", "")) if stroked else []
            color_name = style.get("data-geo-color", "Override")
            if color_name not in color_names:
                color_names.append(color_name)
            strokes.append({
                "color_index": color_names.index(color_name),
                "points": points, "closed": closed, "filled": filled,
                "thickness": float(style.get("stroke-width", 1)) if stroked else 0.0,
                "opacity": opacity,
                "dash": dashes[0] if dashes else 0.0, "gap": dashes[1] if len(dashes) > 1 else 0.0,
            })
        for child in element:
            walk(child, style)

    walk(root, {})
    return color_names, own_color, view_size, strokes


# ---------------------------------------------------------------------------------------------------------------------
# Editor side
# ---------------------------------------------------------------------------------------------------------------------
def _load_toolkit():
    """asset_guard and rail_style, imported from this folder and reloaded so a run starts from their current source."""
    import importlib
    folder = os.path.join(project_folder(), "AI", "Python", "UI")
    if folder not in sys.path:
        sys.path.insert(0, folder)
    import asset_guard as guard
    import rail_style as style
    return importlib.reload(guard), importlib.reload(style)


def srgb_to_linear(value):
    return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4


def build_icon(name, svg_path):
    import unreal
    color_names, own_color, view_size, strokes = parse_svg(svg_path)
    asset_path = f"{ICON_FOLDER}/DA_Icon_{name}"
    icon = unreal.load_asset(asset_path) if unreal.EditorAssetLibrary.does_asset_exist(asset_path) else None
    if not icon:
        asset_guard.created(asset_path)
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.GeoIcon)
        icon = unreal.AssetToolsHelpers.get_asset_tools().create_asset(f"DA_Icon_{name}", ICON_FOLDER, unreal.GeoIcon,
                                                                        factory)
    colors = []
    for color_name in color_names:
        color = unreal.GeoColorParam()
        color.set_editor_property("color", getattr(unreal.GeoColor,
                                                   re.sub(r"(?<!^)(?=[A-Z])", "_", color_name).upper()))
        if own_color:
            color.set_editor_property("override_color", unreal.LinearColor(*[srgb_to_linear(c) for c in own_color], 1))
        colors.append(color)
    asset_guard.write(asset_path, icon, "color", colors[0])
    asset_guard.write(asset_path, icon, "secondary_colors", colors[1:])

    built = []
    for stroke in strokes:
        value = unreal.GeoIconStroke()
        value.set_editor_property("points", [unreal.Vector2D(x, y) for x, y in stroke["points"]])
        value.set_editor_property("closed", stroke["closed"])
        value.set_editor_property("filled", stroke["filled"])
        value.set_editor_property("line_thickness", stroke["thickness"])
        value.set_editor_property("opacity", stroke["opacity"])
        value.set_editor_property("dash_length", stroke["dash"])
        value.set_editor_property("dash_gap", stroke["gap"])
        value.set_editor_property("color_index", stroke["color_index"])
        built.append(value)
    asset_guard.write(asset_path, icon, "view_size", view_size)
    asset_guard.write(asset_path, icon, "strokes", built)
    unreal.EditorAssetLibrary.save_loaded_asset(icon, only_if_is_dirty=False)
    return icon


def icon_literal(icon):
    return f"\"/Script/GeoTrinity.GeoIcon'{icon.get_path_name()}'\""


def assign_ability_icons(icons):
    import unreal
    ability_info = unreal.load_asset(ABILITY_INFO_PATH)
    report = []
    filled = False
    for array_name in PLAYER_ARRAYS:
        rebuilt, array_filled = [], False
        for entry in ability_info.get_editor_property(array_name):
            ability_class = entry.get_editor_property("ability_class")
            class_name = ability_class.get_name()[:-2] if ability_class else ""
            icon = icons.get(ABILITY_ICONS.get(class_name))
            if icon and not entry.get_editor_property("ability_icon"):
                clone = unreal.PlayersGameplayAbilityInfo()
                clone.import_text(entry.export_text())
                clone.import_text(f"(AbilityIcon={icon_literal(icon)})")
                entry = clone
                array_filled = True
            current = entry.get_editor_property("ability_icon")
            report.append(f"{class_name} -> {current.get_name() if current else icon.get_name() if icon else 'NO ICON'}")
            rebuilt.append(entry)
        if array_filled:
            ability_info.set_editor_property(array_name, rebuilt)
            filled = True
    if filled:
        unreal.EditorAssetLibrary.save_loaded_asset(ability_info, only_if_is_dirty=False)
    return report


def with_effect_icons(text, icons):
    """Text of one effect data, its Icon pointing at the icon of its GameplayEffect when EFFECT_ICONS names one and
    the Icon is still empty — an icon set by hand stays."""
    match = re.search(r"GameplayEffect=\"[^\"]*/(\w+)\.\w+'\"", text)
    icon = icons.get(EFFECT_ICONS.get(match.group(1))) if match else None
    if not icon:
        return text
    return re.sub(r"Icon=None", f"Icon={icon_literal(icon)}", text, count=1)


def assign_effect_icons(icons):
    """Every ability Blueprint's effect data — a struct, an instanced struct or an array of them on its CDO — whose
    GameplayEffect has an icon in EFFECT_ICONS gets that icon."""
    import unreal
    report = []
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in registry.get_assets_by_path(ABILITY_FOLDER, True):
        if str(data.asset_class_path.asset_name) != "Blueprint":
            continue
        blueprint = unreal.load_asset(str(data.package_name))
        generated = blueprint.generated_class() if blueprint else None
        if not generated or not unreal.MathLibrary.class_is_child_of(generated, unreal.GameplayAbility):
            continue
        cdo = unreal.get_default_object(generated)
        changed = False
        for name in dir(cdo):
            if name.startswith("_"):
                continue
            try:
                value = cdo.get_editor_property(name)
            except Exception:
                continue
            if isinstance(value, (unreal.GameplayEffectData, unreal.InstancedStruct)):
                text = value.export_text()
                new_text = with_effect_icons(text, icons)
                if new_text != text:
                    fresh = type(value)()
                    fresh.import_text(new_text)
                    cdo.set_editor_property(name, fresh)
                    changed = True
                    report.append(f"{blueprint.get_name()}.{name}")
            elif isinstance(value, unreal.Array) and len(value) and isinstance(value[0], unreal.InstancedStruct):
                rebuilt, array_changed = [], False
                for entry in value:
                    text = entry.export_text()
                    new_text = with_effect_icons(text, icons)
                    if new_text != text:
                        entry = unreal.InstancedStruct()
                        entry.import_text(new_text)
                        array_changed = True
                    rebuilt.append(entry)
                if array_changed:
                    cdo.set_editor_property(name, rebuilt)
                    changed = True
                    report.append(f"{blueprint.get_name()}.{name}")
        if changed:
            unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
            unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    return report


def assign_build_tab_icons(icons):
    """The gem loadout page's rename, remove and add build tab icons, while still empty — an icon set by hand stays."""
    import unreal
    blueprint = unreal.load_asset(GEM_LOADOUT_PATH)
    cdo = unreal.get_default_object(blueprint.generated_class())
    report = []
    for property_name, icon_name in BUILD_TAB_ICONS.items():
        if not cdo.get_editor_property(property_name) and icons.get(icon_name):
            cdo.set_editor_property(property_name, icons[icon_name])
            report.append(f"{property_name} -> {icon_name}")
    if report:
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    return report


def use_icon_widget_in_ability_slot():
    """The slot's Icon was an Image showing a texture; it draws the ability's UGeoIcon now."""
    import unreal
    icon = rail_style.UTIL.find_widget(unreal.load_asset(ABILITY_SLOT_PATH), "Icon")
    if not isinstance(icon, unreal.GeoIconImage):
        slot = unreal.load_asset(ABILITY_SLOT_PATH)
        rail_style.swap_widget_class(slot, "Icon", unreal.GeoIconImage)
        rail_style.UTIL.commit_tree(slot)
        rail_style.reload_blueprint(slot)


def run():
    import unreal
    global asset_guard, rail_style
    asset_guard, rail_style = _load_toolkit()
    use_icon_widget_in_ability_slot()
    folder = source_folder()
    icons = {}
    for file_name in sorted(os.listdir(folder)):
        if file_name.endswith(".svg"):
            name = file_name[:-4]
            icons[name] = build_icon(name, os.path.join(folder, file_name))
    report = [f"{len(icons)} icons built"]
    report += assign_ability_icons(icons)
    report += ["effects: " + ", ".join(assign_effect_icons(icons))]
    report += ["build tabs: " + ", ".join(assign_build_tab_icons(icons))]
    report += ["kept by hand: " + line for line in asset_guard.report()]
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "import_icons.txt")
    open(output, "w").write("\n".join(report))


if __name__ == "__main__":
    run()
