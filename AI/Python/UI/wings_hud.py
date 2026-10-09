"""
The in-game HUD in the Wings look: the player's wing bottom-left, the team list on the left, the stats top-right, the
boss rail across the top, framed ability slots with a cooldown sweep and framed status tiles.
- DA_Meter_*: the meter styles (player, team and boss health, class ring, cooldown sweep, status time).
- WBP_PlayerWing / WBP_TeamRow (UGeoPlayerCardWidget), WBP_TeamList, WBP_StatsPanel, in /Game/HUD/Wings: built once;
  the wing is minimal (class shape in its gauge ring, health meter with the health over the max on it, no frame:
  an older wing is stripped to that). WBP_StatsPanel is the stat counter: a bare grid the widget fills with every
  player's figures, its header icons, and IA_ToggleStatsDetail (C, d-pad right) switching its detail.
- BP_BossHealthBar: a BossRail holding the shape, name, meter, percent and the fight timer; the legacy bar collapsed.
- WBP_AbilitySlot: Stack wrapped in SlotFrame, CooldownSweep image replaced by the CooldownMeter, CountText in the
  outlined Overlay role bottom-right.
- WBP_StatusBar: its tile and meter styles. WBP_MainOverlay: PlayerCard, TeamList, StatsPanel placed, the abilities
  and the status tiles' own zone right of them in one bottom-centre row, LifeBarGroup collapsed.

Never overwrites a hand edit: new widgets are built only when created, widgets are added only when absent, and every
value goes through asset_guard.write (kept values are listed in AI/Output/wings_hud.txt).
Usage: run via MCP execute_script, after ui_theme.py.
"""
import importlib
import os
import sys

import unreal

UI_SCRIPTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "AI", "Python", "UI")
if UI_SCRIPTS not in sys.path:
    sys.path.insert(0, UI_SCRIPTS)
import ability_page
import ui_theme

ability_page = importlib.reload(ability_page)
asset_guard = ability_page.asset_guard
add = ability_page.add
write_slot = ability_page.write_slot
srgb = ui_theme.srgb

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
ROLE = unreal.GeoTextRole
STYLE_DIR = "/Game/HUD/Style"
WINGS_DIR = "/Game/HUD/Wings"
OVERLAY_PATH = "/Game/HUD/WBP_MainOverlay"
SLOT_PATH = "/Game/HUD/AbilityBar/WBP_AbilitySlot"
STATUS_BAR_PATH = "/Game/HUD/StatusBar/WBP_StatusBar"
BOSS_BAR_PATH = "/Game/HUD/ProgressBar/BP_BossHealthBar"
CENTER = unreal.VerticalAlignment.V_ALIGN_CENTER
FILL = unreal.SlateChildSize(1.0, unreal.SlateSizeRule.FILL)
# The status tiles' own zone right of the abilities: five tiles wide (56 + 6 gap each), wrapping to a new row past it.
STATUS_ZONE_WIDTH = 310.0
STATUS_ZONE_GAP = 32.0
SHAPE = unreal.GeoMeterShape

METER_STYLES = {
    # The player's life: class-coloured fill, the shield overhanging it, quarter ticks, a white outline in a class glow.
    "DA_Meter_PlayerHealth": dict(
        shape=SHAPE.BAR, size=unreal.Vector2D(300, 24), track_color=srgb("FFFFFF", .08),
        low_track_color=srgb("FFFFFF", .08), fill_color=srgb("FFFFFF"), overhang_size=6.0,
        overhang_fill_color=srgb("5CC8FF", .55), overhang_line_color=srgb("5CC8FF"), overhang_line_thickness=2.0,
        tick_parts=4, tick_color=srgb("04040A", .6), tick_width=1.0, outline_color=srgb("FFFFFF"),
        outline_thickness=2.0, outline_glow_color=srgb("FFFFFF", .45), outline_glow_thickness=12.0),
    # A teammate's life: thin, glowing in the class colour, the track turning red when low.
    "DA_Meter_TeamHealth": dict(
        shape=SHAPE.BAR, size=unreal.Vector2D(150, 6), track_color=srgb("FFFFFF", .12),
        low_track_color=srgb("FF5A6E", .35), low_threshold=.35, fill_color=srgb("FFFFFF"), glow_thickness=6.0,
        glow_opacity=.6, overhang_size=3.0, overhang_fill_color=srgb("5CC8FF", .55),
        overhang_line_color=srgb("5CC8FF"), overhang_line_thickness=1.0),
    # The boss's life: red, glowing, cut in thirds by white ticks standing out of it.
    "DA_Meter_BossHealth": dict(
        shape=SHAPE.BAR, size=unreal.Vector2D(400, 10), track_color=srgb("FFFFFF", .08),
        low_track_color=srgb("FFFFFF", .08), fill_color=srgb("FF5A6E"), glow_thickness=10.0, glow_opacity=.6,
        tick_parts=3, tick_color=srgb("FFFFFF"), tick_width=2.0, tick_overhang=3.0),
    # The class gauge around the player's shape.
    "DA_Meter_ClassRing": dict(
        shape=SHAPE.RING, size=unreal.Vector2D(76, 76), ring_thickness=4.0, track_color=srgb("FFFFFF", .15),
        fill_color=srgb("FFFFFF")),
    # The dark sweep over an ability cooling down.
    "DA_Meter_Cooldown": dict(shape=SHAPE.SWEEP, size=unreal.Vector2D(84, 84), fill_color=srgb("04040A", .8)),
    # The time left along a status tile's bottom edge.
    "DA_Meter_StatusTime": dict(
        shape=SHAPE.BAR, size=unreal.Vector2D(56, 3), track_color=srgb("000000", 0.0),
        low_track_color=srgb("000000", 0.0), fill_color=srgb("F4EEFF")),
}


def meter(name):
    return unreal.load_asset(f"{STYLE_DIR}/{name}")


def frame(name):
    return unreal.load_asset(f"{STYLE_DIR}/{name}")


def build_meter_styles():
    for name, values in METER_STYLES.items():
        path = f"{STYLE_DIR}/{name}"
        style, created = ui_theme.create_or_load(path, unreal.GeoMeterStyle)
        if created:
            asset_guard.created(path)
        for key, value in values.items():
            asset_guard.write(path, style, key, value)
        unreal.EditorAssetLibrary.save_loaded_asset(style)


def create_widget(path, parent_class):
    """The widget blueprint at path; None when it already exists, so a hand-edited layout is never rebuilt."""
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return None
    asset_guard.created(path)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, f"/Script/GeoTrinityUI.{parent_class}"))
    folder, name = path.rsplit("/", 1)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.WidgetBlueprint, factory)


def finish(wbp):
    UTIL.commit_tree(wbp)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)


def text(role, value, **extra):
    return dict(role=role, text=unreal.Text(value), **extra)


def scrolled(wbp, path, content, scroll):
    """Puts content in a GeoScrollBox named scroll that fills the room content's parent leaves, so whatever grows with
    data scrolls inside its own area instead of spilling over its neighbours. Content's padding moves to the scroll box."""
    if not UTIL.find_widget(wbp, scroll):
        widget = UTIL.find_widget(wbp, content)
        parent = widget.get_parent()
        padding = widget.get_editor_property("slot").get_editor_property("padding")
        UTIL.construct_widget_in_tree(wbp, unreal.GeoScrollBox, scroll, True)
        UTIL.attach_widget(wbp, parent.get_name(), scroll, parent.get_child_index(widget))
        UTIL.attach_widget(wbp, scroll, content)
        write_slot(path, UTIL.find_widget(wbp, scroll).get_editor_property("slot"), {"padding": padding})
    write_slot(path, UTIL.find_widget(wbp, scroll).get_editor_property("slot"), {"size": FILL})


def remove_unless_edited(wbp, path, names):
    """Removes the widgets names, all of them or none: none while one holds a value changed by hand. Returns those hand
    edits."""
    edited = [line for line in asset_guard.hand_edits(path) if any(f".{name}:" in line for name in names)]
    if not edited:
        for name in names:
            if UTIL.find_widget(wbp, name):
                UTIL.remove_widget(wbp, name)
    return edited


# --- New widgets ------------------------------------------------------------------------------------------------------
RETIRED_WING_WIDGETS = ("NameText", "RoleText", "NameRow", "NameScale", "CurrentHealthText", "TitleRow",
                        "MaxHealthText", "ShieldText", "GaugeText", "DetailRow")


def build_player_wing():
    """Minimal: the class shape inside its gauge ring, then the health meter; no frame, no text."""
    path = f"{WINGS_DIR}/WBP_PlayerWing"
    wbp = create_widget(path, "GeoPlayerCardWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.HorizontalBox, "WingRow")
        _, badge_slot = add(wbp, path, unreal.Overlay, "Badge", "WingRow")
        write_slot(path, badge_slot, {"vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoMeter, "GaugeRing", "Badge", {"meter_style": meter("DA_Meter_ClassRing"), "fill": .6})
        _, shape_slot = add(wbp, path, unreal.GeoShape, "ClassShape", "Badge", {"size": 48.0, "filled": True})
        write_slot(path, shape_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_CENTER,
                                      "vertical_alignment": CENTER})
        _, info_slot = add(wbp, path, unreal.VerticalBox, "Info", "WingRow")
        write_slot(path, info_slot, {"size": FILL, "padding": unreal.Margin(20, 0, 0, 0), "vertical_alignment": CENTER})
        add(wbp, path, unreal.GeoMeter, "HealthMeter", "Info",
            {"meter_style": meter("DA_Meter_PlayerHealth"), "overhang": .12})
        finish(wbp)
    wbp = unreal.load_asset(path)
    strip_player_wing(wbp)
    stack_health_text(wbp, path)
    finish(wbp)
    return wbp


def strip_player_wing(wbp):
    """The wing's first version sat in a WingFrame with name, role, health and stat lines: only badge and meter stay."""
    for name in RETIRED_WING_WIDGETS:
        if UTIL.find_widget(wbp, name):
            UTIL.remove_widget(wbp, name)
    if UTIL.find_widget(wbp, "WingFrame"):
        UTIL.set_root_widget(wbp, "WingRow")


def stack_health_text(wbp, path):
    """HealthText, the health over the max in the outlined Overlay role, centred on HealthMeter in a HealthStack.

    The meter keeps its own height, centred, so the taller text overhangs the bar's top and bottom edges."""
    if not UTIL.find_widget(wbp, "HealthStack"):
        health_meter = UTIL.find_widget(wbp, "HealthMeter")
        parent = health_meter.get_parent()
        UTIL.construct_widget_in_tree(wbp, unreal.Overlay, "HealthStack", True)
        UTIL.attach_widget(wbp, parent.get_name(), "HealthStack", parent.get_child_index(health_meter))
        UTIL.attach_widget(wbp, "HealthStack", "HealthMeter")
    write_slot(path, UTIL.find_widget(wbp, "HealthStack").get_editor_property("slot"),
               {"padding": unreal.Margin(0, 12, 0, 12)})
    write_slot(path, UTIL.find_widget(wbp, "HealthMeter").get_editor_property("slot"),
               {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_FILL, "vertical_alignment": CENTER})
    _, text_slot = add(wbp, path, unreal.GeoText, "HealthText", "HealthStack", text(ROLE.OVERLAY, "1000 / 1000"))
    write_slot(path, text_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_CENTER,
                                 "vertical_alignment": CENTER})


def sized(wbp, path, parent, name, width):
    """A SizeBox of width named name under parent, vertically centred in its row."""
    _, box_slot = add(wbp, path, unreal.SizeBox, name, parent,
                      {"override_width_override": True, "width_override": width})
    write_slot(path, box_slot, {"vertical_alignment": CENTER, "padding": unreal.Margin(0, 0, 10, 0)})


def build_team_row():
    path = f"{WINGS_DIR}/WBP_TeamRow"
    wbp = create_widget(path, "GeoPlayerCardWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.HorizontalBox, "Row")
        sized(wbp, path, "Row", "ShapeBox", 16.0)
        add(wbp, path, unreal.GeoShape, "ClassShape", "ShapeBox", {"size": 14.0, "filled": True})
        sized(wbp, path, "Row", "NameBox", 110.0)
        add(wbp, path, unreal.GeoText, "NameText", "NameBox", text(ROLE.BODY, "Teammate"))
        sized(wbp, path, "Row", "MeterBox", 150.0)
        add(wbp, path, unreal.GeoMeter, "HealthMeter", "MeterBox", {"meter_style": meter("DA_Meter_TeamHealth")})
        sized(wbp, path, "Row", "PercentBox", 44.0)
        add(wbp, path, unreal.GeoText, "HealthPercentText", "PercentBox",
            text(ROLE.MONO, "100%", justification=unreal.TextJustify.RIGHT))
        finish(wbp)
    return unreal.load_asset(path)


def build_team_list(row):
    path = f"{WINGS_DIR}/WBP_TeamList"
    wbp = create_widget(path, "GeoTeamListWidget")
    if wbp:
        UTIL.set_root_panel(wbp, unreal.VerticalBox, "RowBox")
        finish(wbp)
        asset_guard.write(path, unreal.get_default_object(wbp.generated_class()), "row_class", row.generated_class())
        finish(wbp)
    return unreal.load_asset(path)


# Two columns, read row by row: damage on the left, healing on the right.
# The stat counter's header icons, by EGeoStatGroup (DA_Icon_<name>, from SourceArt/Icons).
STAT_GROUP_ICONS = {"DAMAGE": "Stat_Damage", "HEALING": "Stat_Healing", "DAMAGE_TAKEN": "Stat_DamageTaken"}
# The framed two-column panel the counter replaced.
RETIRED_STATS_WIDGETS = ("StatsFrame", "StatGrid")
STATS_TOGGLE_ACTION = "/Game/Input/InputActions/IA_ToggleStatsDetail"
STATS_TOGGLE_KEYS = ("C", "Gamepad_DPad_Right")
INPUT_MAPPING_PATH = "/Game/Input/BP_GeoInputMapping"
PLAYER_CONTROLLER_PATH = "/Game/Characters/Playable/BP_GeoPlayerController"


def build_stats_panel():
    """WBP_StatsPanel (UGeoStatsPanelWidget), the stat counter: a bare StatsGrid the widget fills with every player's
    figures, the MORE / LESS hint under it, its header icons. The older framed panel is emptied first, unless a value of
    it was changed by hand: then it is left alone and listed."""
    path = f"{WINGS_DIR}/WBP_StatsPanel"
    wbp = create_widget(path, "GeoStatsPanelWidget")
    if not wbp:
        wbp = unreal.load_asset(path)
        if UTIL.find_widget(wbp, "StatsFrame"):
            if asset_guard.hand_edits(path):
                return wbp
    if not UTIL.find_widget(wbp, "StatsBody"):
        UTIL.set_root_panel(wbp, unreal.VerticalBox, "StatsBody")
        finish(wbp)
    add(wbp, path, unreal.GridPanel, "StatsGrid", "StatsBody")
    _, hint_slot = add(wbp, path, unreal.GeoText, "ToggleHintText", "StatsBody", text(ROLE.MONO, "[C] MORE"))
    write_slot(path, hint_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_RIGHT})
    finish(wbp)

    defaults = unreal.get_default_object(wbp.generated_class())
    icon = lambda name: unreal.load_asset(f"/Game/HUD/Icons/DA_Icon_{name}")
    asset_guard.write(path, defaults, "group_icons",
                      {getattr(unreal.GeoStatGroup, group): icon(name) for group, name in STAT_GROUP_ICONS.items()})
    asset_guard.write(path, defaults, "fight_time_icon", icon("Stat_FightTime"))
    finish(wbp)
    add_stats_toggle_input()
    return wbp


def add_input(action_path, mappable_name, display_name, keys, controller_values):
    """The input action at action_path (made when missing, rebindable as mappable_name), on keys in the gameplay
    mapping, and controller_values ({property: value}, the action being "action") written on BP_GeoPlayerController."""
    action = unreal.load_asset(action_path) if unreal.EditorAssetLibrary.does_asset_exist(action_path) else None
    if not action:
        folder, name = action_path.rsplit("/", 1)
        action = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.InputAction,
                                                                         unreal.InputAction_Factory())
        settings = unreal.new_object(unreal.PlayerMappableKeySettings, action)
        settings.set_editor_property("name", mappable_name)
        settings.set_editor_property("display_name", unreal.Text(display_name))
        action.set_editor_property("player_mappable_key_settings", settings)
        unreal.EditorAssetLibrary.save_loaded_asset(action)

    mapping = unreal.load_asset(INPUT_MAPPING_PATH)
    mapped = [m.get_editor_property("key").export_text() for m in
              mapping.get_editor_property("default_key_mappings").get_editor_property("mappings")
              if m.get_editor_property("action") == action]
    for key_name in keys:
        if not any(key_name in key for key in mapped):
            key = unreal.Key()
            key.import_text(key_name)
            mapping.map_key(action, key)
    # map_key leaves the asset clean, so only a forced save keeps the mappings.
    unreal.EditorAssetLibrary.save_loaded_asset(mapping, only_if_is_dirty=False)

    controller = unreal.load_asset(PLAYER_CONTROLLER_PATH)
    defaults = unreal.get_default_object(controller.generated_class())
    for key, value in controller_values.items():
        asset_guard.write(PLAYER_CONTROLLER_PATH, defaults, key, action if value == "action" else value)
    unreal.BlueprintEditorLibrary.compile_blueprint(controller)
    unreal.EditorAssetLibrary.save_loaded_asset(controller, only_if_is_dirty=False)


def add_stats_toggle_input():
    """IA_ToggleStatsDetail, rebindable as ToggleStats, on STATS_TOGGLE_KEYS, named on BP_GeoPlayerController, whose
    OnToggleStatsDetail switches the counter's table."""
    add_input(STATS_TOGGLE_ACTION, "ToggleStats", "Stats detail", STATS_TOGGLE_KEYS,
              {"toggle_stats_detail_action": "action"})


# --- Existing widgets -------------------------------------------------------------------------------------------------
def canvas_layout(anchors, offsets, alignment):
    """Canvas slot layout: anchors (min x, min y, max x, max y), offsets (left, top, right, bottom), alignment."""
    layout = unreal.AnchorData()
    layout.set_editor_property("anchors", unreal.Anchors(unreal.Vector2D(*anchors[:2]), unreal.Vector2D(*anchors[2:])))
    layout.set_editor_property("offsets", unreal.Margin(*offsets))
    layout.set_editor_property("alignment", unreal.Vector2D(*alignment))
    return layout


def restyle_boss_bar():
    path = BOSS_BAR_PATH
    wbp = unreal.load_asset(path)
    timer = UTIL.find_widget(wbp, "FightTimerText")
    root = UTIL.find_widget(wbp, "WBP_ProgressBar").get_parent()
    _, rail_slot = add(wbp, path, unreal.GeoFrame, "BossRail", root.get_name(),
                          {"frame_style": frame("DA_Frame_Wing"), "padding": unreal.Margin(20, 4, 20, 4)})
    write_slot(path, rail_slot, {"layout_data": canvas_layout((0, 0, 1, 0), (0, 0, 0, 35), (0, 0))})
    add(wbp, path, unreal.HorizontalBox, "RailRow", "BossRail")
    _, shape_slot = add(wbp, path, unreal.GeoShape, "BossShape", "RailRow",
                        {"sides": 6, "rotation": 30.0, "size": 22.0, "filled": True, "color": srgb("FF5A6E")})
    write_slot(path, shape_slot, {"vertical_alignment": CENTER})
    _, name_slot = add(wbp, path, unreal.GeoText, "BossNameText", "RailRow", text(ROLE.BUTTON, "HEX BOSS"))
    write_slot(path, name_slot, {"padding": unreal.Margin(18, 0, 18, 0), "vertical_alignment": CENTER})
    _, meter_slot = add(wbp, path, unreal.GeoMeter, "HealthMeter", "RailRow",
                        {"meter_style": meter("DA_Meter_BossHealth")})
    write_slot(path, meter_slot, {"size": FILL, "vertical_alignment": CENTER})
    _, percent_slot = add(wbp, path, unreal.GeoText, "HealthPercentText", "RailRow", text(ROLE.MONO, "100%"))
    write_slot(path, percent_slot, {"padding": unreal.Margin(18, 0, 18, 0), "vertical_alignment": CENTER})
    if timer.get_parent().get_name() != "RailRow":
        UTIL.attach_widget(wbp, "RailRow", "FightTimerText")
    write_slot(path, UTIL.find_widget(wbp, "FightTimerText").get_editor_property("slot"),
               {"vertical_alignment": CENTER})
    asset_guard.write(path, UTIL.find_widget(wbp, "WBP_ProgressBar"), "visibility", unreal.SlateVisibility.COLLAPSED)
    finish(wbp)


def restyle_ability_slot():
    path = SLOT_PATH
    wbp = unreal.load_asset(path)
    if not UTIL.find_widget(wbp, "SlotFrame"):
        UTIL.construct_widget_in_tree(wbp, unreal.GeoFrame, "SlotFrame", True)
        UTIL.attach_widget(wbp, "SlotFrame", "Stack")
        UTIL.attach_widget(wbp, "Square", "SlotFrame")
    slot_frame = UTIL.find_widget(wbp, "SlotFrame")
    for key, value in dict(frame_style=frame("DA_Frame_AbilitySlot"), padding=unreal.Margin(0, 0, 0, 0)).items():
        asset_guard.write(path, slot_frame, key, value)
    if UTIL.find_widget(wbp, "CooldownSweep"):
        UTIL.remove_widget(wbp, "CooldownSweep")
    if not UTIL.find_widget(wbp, "CooldownMeter"):
        UTIL.construct_widget_in_tree(wbp, unreal.GeoMeter, "CooldownMeter", True)
        UTIL.attach_widget(wbp, "Stack", "CooldownMeter", 1)
    cooldown = UTIL.find_widget(wbp, "CooldownMeter")
    asset_guard.write(path, cooldown, "meter_style", meter("DA_Meter_Cooldown"))
    asset_guard.write(path, cooldown, "fill", 0.0)
    write_slot(path, cooldown.get_editor_property("slot"),
               {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_FILL, "vertical_alignment": CENTER})
    square = UTIL.find_widget(wbp, "Square")
    # ability_bar.py built the slot 64 wide before the ledger existed.
    for key, value in dict(override_width_override=True, width_override=84.0, override_height_override=True,
                           height_override=84.0).items():
        asset_guard.write(path, square, key, value, earlier=(64.0,))
    overlay_count_text(wbp, path)
    finish(wbp)


def overlay_count_text(wbp, path):
    """CountText, the deploy charges left, in the outlined Overlay role like the health, in the slot's bottom-right
    corner. Replaces the older plain TextBlock of that name.

    Scaled down from that corner so it clears the cooldown countdown centred above it; the negative bottom padding
    eats the line's descender room, putting the digit as far from the bottom edge as from the side."""
    count = UTIL.find_widget(wbp, "CountText")
    if count and not isinstance(count, unreal.GeoText):
        UTIL.remove_widget(wbp, "CountText")
    count, count_slot = add(wbp, path, unreal.GeoText, "CountText", "Stack", text(ROLE.OVERLAY, "3"))
    asset_guard.write(path, count, "render_transform_pivot", unreal.Vector2D(1, 1))
    asset_guard.write(path, count, "render_transform", unreal.WidgetTransform(scale=unreal.Vector2D(.8, .8)))
    write_slot(path, count_slot, {"horizontal_alignment": unreal.HorizontalAlignment.H_ALIGN_RIGHT,
                                  "vertical_alignment": unreal.VerticalAlignment.V_ALIGN_BOTTOM,
                                  "padding": unreal.Margin(0, 0, 6, -6)})


def style_status_bar():
    path = STATUS_BAR_PATH
    wbp = unreal.load_asset(path)
    defaults = unreal.get_default_object(wbp.generated_class())
    asset_guard.write(path, defaults, "tile_style", frame("DA_Frame_StatusTile"))
    asset_guard.write(path, defaults, "debuff_tile_style", frame("DA_Frame_StatusDebuff"))
    asset_guard.write(path, defaults, "time_meter_style", meter("DA_Meter_StatusTime"))
    finish(wbp)


def place(wbp, path, root, name, widget_class, layout, fixed_size=False, earlier=()):
    """Pins name to a point of the overlay canvas, never stretched over a band its content could outgrow: sized by its
    content, or to the size in layout's offsets when fixed_size (content that must fit it, like a name shrinking).
    earlier: layouts an older script set there."""
    widget = UTIL.find_widget(wbp, name)
    if not widget:
        widget = UTIL.construct_widget_in_tree(wbp, widget_class, name, True)
        UTIL.attach_widget(wbp, root, name)
    slot = widget.get_editor_property("slot")
    asset_guard.write(path, slot, "layout_data", layout, earlier)
    asset_guard.write(path, slot, "auto_size", not fixed_size)


def place_action_row(wbp, path, root):
    """ActionRow, bottom-centre: ActionBalance, the AbilityBar, then the StatusZone holding the StatusBar. The status
    tiles keep their own zone a gap right of the abilities whatever the class's ability count, and the balance spacer
    (the zone plus the gap) keeps the abilities centred on the screen: retune both together."""
    place(wbp, path, root, "ActionRow", unreal.HorizontalBox,
          canvas_layout((0.5, 1, 0.5, 1), (0, -14, 0, 0), (0.5, 1)))
    _, balance_slot = add(wbp, path, unreal.Spacer, "ActionBalance", "ActionRow",
                          {"size": unreal.Vector2D(STATUS_ZONE_WIDTH + STATUS_ZONE_GAP, 1)})
    _, zone_slot = add(wbp, path, unreal.SizeBox, "StatusZone", "ActionRow",
                       {"width_override": STATUS_ZONE_WIDTH, "override_width_override": True})
    for name, parent, index in [("AbilityBar", "ActionRow", 1), ("StatusBar", "StatusZone", 0)]:
        if UTIL.find_widget(wbp, name).get_parent().get_name() != parent:
            UTIL.attach_widget(wbp, parent, name, index)
    bottom = unreal.VerticalAlignment.V_ALIGN_BOTTOM
    write_slot(path, balance_slot, {"vertical_alignment": bottom})
    write_slot(path, UTIL.find_widget(wbp, "AbilityBar").get_editor_property("slot"), {"vertical_alignment": bottom})
    write_slot(path, zone_slot, {"vertical_alignment": bottom,
                                 "padding": unreal.Margin(STATUS_ZONE_GAP, 0, 0, 0)})


def build_overlay(wing, team_list, stats):
    path = OVERLAY_PATH
    wbp = unreal.load_asset(path)
    row = UTIL.find_widget(wbp, "ActionRow") or UTIL.find_widget(wbp, "AbilityBar")
    root = row.get_parent().get_name()
    place_action_row(wbp, path, root)
    place(wbp, path, root, "PlayerCard", wing.generated_class(),
          canvas_layout((0, 1, 0, 1), (28, -28, 470, 120), (0, 1)), fixed_size=True)
    place(wbp, path, root, "TeamList", team_list.generated_class(),
          canvas_layout((0, 0, 0, 0), (6, 199, 360, 200), (0, 0)))
    place(wbp, path, root, "StatsPanel", stats.generated_class(),
          canvas_layout((1, 0, 1, 0), (-1, 40, 340, 120), (1, 0)))
    life = UTIL.find_widget(wbp, "LifeBarGroup")
    if life:
        asset_guard.write(path, life, "visibility", unreal.SlateVisibility.COLLAPSED)
    finish(wbp)


def run():
    build_meter_styles()
    wing = build_player_wing()
    team_list = build_team_list(build_team_row())
    stats = build_stats_panel()
    restyle_boss_bar()
    restyle_ability_slot()
    style_status_bar()
    build_overlay(wing, team_list, stats)
    kept = asset_guard.report()
    output = os.path.join(unreal.Paths.project_dir(), "AI", "Output", "wings_hud.txt")
    open(output, "w", encoding="utf-8").write("\n".join(["OK"] + ["kept by hand: " + line for line in kept]))


if __name__ == "__main__":
    run()
