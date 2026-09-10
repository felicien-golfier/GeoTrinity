"""
Leaderboard end to end: build WBP_Leaderboard, add its entry to the main menu, and put the fight timer beside the
boss health bar (reparenting that bar to the C++ class holding the timer).
Usage: run via MCP execute_script, AFTER list_panel.py — the panel is built on the shared list frame that makes.
Re-run-safe end to end. Adjust the example call at the bottom.
"""
import unreal

TIMER_NAME = 'FightTimerText'
TIMER_FONT_SIZE = 36
# Fraction of the screen between the bar's right edge and the timer, in the bar's own anchor space.
TIMER_GAP = 0.01


def create_widget_blueprint(name, folder, parent_class_path):
    full_path = f"{folder}/{name}"
    existing = unreal.load_asset(full_path)
    if existing:
        return existing
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, parent_class_path))
    wbp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.WidgetBlueprint, factory)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)
    return wbp


def inspect_widget_tree(wbp):
    """Log a widget BP's full tree (types, names, slots) — read it back via the output-log tool."""
    unreal.GeoWidgetBuilderUtil.get_default_object().inspect_widget_blueprint(wbp)


def save_assets(*asset_paths):
    """Builders mutate in memory only; nothing survives the editor closing until this runs."""
    for path in asset_paths:
        unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)


def read_font(asset_path, text_name):
    """The font a menu authored, so a built header is set in the same type as the menus around it."""
    text = unreal.GeoWidgetBuilderUtil.get_default_object().find_widget(unreal.load_asset(asset_path), text_name)
    if not text:
        raise LookupError(f"{asset_path} holds no text named '{text_name}'")
    return text.get_editor_property('font')


def setup_leaderboard_menu(panel_name, panel_folder, panel_parent_class_path, list_frame_path, button_class_path,
                           title_font_path, title_font_widget, main_menu_path, canvas_name, buttons_box_name):
    panel = create_widget_blueprint(panel_name, panel_folder, panel_parent_class_path)

    button_class = unreal.load_class(None, button_class_path)
    util = unreal.GeoHudWidgetBuilderUtil.get_default_object()
    util.build_leaderboard_widget(panel, unreal.load_asset(list_frame_path).generated_class(), button_class,
                                  read_font(title_font_path, title_font_widget))

    main_menu = unreal.load_asset(main_menu_path)
    util.add_panel_entry_to_main_menu(main_menu, canvas_name, buttons_box_name, button_class,
                                      'LeaderboardButton', unreal.Text('Leaderboard'),
                                      'LeaderboardWidget', panel.generated_class())

    # A list panel is the whole screen, not a box in the middle of it — the entry builder centers it auto-sized.
    slot = unreal.GeoWidgetBuilderUtil.get_default_object().find_widget(main_menu, 'LeaderboardWidget').get_editor_property('slot')
    slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(1.0, 1.0)))
    slot.set_auto_size(False)
    slot.set_offsets(unreal.Margin())
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))
    unreal.GeoWidgetBuilderUtil.get_default_object().commit_tree(main_menu)

    inspect_widget_tree(main_menu)
    save_assets(f"{panel_folder}/{panel_name}", main_menu_path)


def add_leaderboard_to_pause_menu(menu_path, panel_path, button_class_path, buttons_box_name, panel_parent_name,
                                  before_button_name):
    """Same leaderboard panel as the main menu, in the in-game pause menu. That menu is an Overlay tree, not a
    canvas, so the entry goes in through the tree primitives rather than add_panel_entry_to_main_menu."""
    menu = unreal.load_asset(menu_path)
    util = unreal.GeoWidgetBuilderUtil.get_default_object()

    button = util.construct_widget_in_tree(menu, unreal.load_class(None, button_class_path), 'LeaderboardButton', True)
    button.set_editor_property('label', unreal.Text('Leaderboard'))
    buttons_box = util.find_widget(menu, buttons_box_name)
    before = util.find_widget(menu, before_button_name)
    util.attach_widget(menu, buttons_box_name, 'LeaderboardButton', buttons_box.get_child_index(before))

    panel_class = unreal.load_asset(panel_path).generated_class()
    panel = util.construct_widget_in_tree(menu, panel_class, 'LeaderboardWidget', True)
    # Off until its button opens it: the menu collapses it on construct too, but only once that C++ is built.
    panel.set_visibility(unreal.SlateVisibility.COLLAPSED)
    slot = util.attach_widget(menu, panel_parent_name, 'LeaderboardWidget', -1)
    if isinstance(slot, unreal.OverlaySlot):
        slot.set_editor_property('horizontal_alignment', unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_editor_property('vertical_alignment', unreal.VerticalAlignment.V_ALIGN_FILL)

    util.commit_tree(menu)
    inspect_widget_tree(menu)
    save_assets(menu_path)


def add_fight_timer_to_boss_bar(bar_path, bar_parent_class_path, health_bar_name):
    """Reparent the boss bar to the timer-carrying C++ class and drop the timer text beside its health bar."""
    bar = unreal.load_asset(bar_path)
    unreal.BlueprintEditorLibrary.reparent_blueprint(bar, unreal.load_class(None, bar_parent_class_path))

    util = unreal.GeoWidgetBuilderUtil.get_default_object()
    health_bar = util.find_widget(bar, health_bar_name)
    if not health_bar:
        inspect_widget_tree(bar)
        raise LookupError(f"{bar_path} holds no widget named '{health_bar_name}' — its tree is logged above")
    parent = health_bar.get_parent()

    timer = util.add_widget_to_panel(bar, parent.get_name(), unreal.TextBlock.static_class(), TIMER_NAME,
                                     unreal.Margin())
    timer.set_text(unreal.Text('0:00.0'))
    font = timer.get_editor_property('font')
    font.size = TIMER_FONT_SIZE
    timer.set_editor_property('font', font)

    # The bar stretches between anchors rather than sitting on a pixel rect, so the timer is anchored to its right
    # edge and centred on its height. Auto-size leaves the text free to be taller than the bar is thin.
    bar_slot = health_bar.get_editor_property('slot')
    timer_slot = timer.get_editor_property('slot')
    if isinstance(bar_slot, unreal.CanvasPanelSlot) and isinstance(timer_slot, unreal.CanvasPanelSlot):
        anchors = bar_slot.get_layout().anchors
        edge = unreal.Vector2D(anchors.maximum.x + TIMER_GAP, (anchors.minimum.y + anchors.maximum.y) * 0.5)
        timer_slot.set_anchors(unreal.Anchors(minimum=edge, maximum=edge))
        timer_slot.set_offsets(unreal.Margin())
        timer_slot.set_alignment(unreal.Vector2D(0.0, 0.5))
        timer_slot.set_auto_size(True)

    util.commit_tree(bar)
    inspect_widget_tree(bar)
    save_assets(bar_path)


# --- Example: the GeoTrinity leaderboard and boss-bar timer ---
setup_leaderboard_menu(
    panel_name='WBP_Leaderboard',
    panel_folder='/Game/HUD/MainMenu',
    panel_parent_class_path='/Script/GeoTrinityUI.GeoLeaderboardWidget',
    list_frame_path='/Game/HUD/WBP_ListPanel',
    button_class_path='/Game/HUD/WBP_GeoButton.WBP_GeoButton_C',
    title_font_path='/Game/HUD/MainMenu/WBP_CreateServerWidget',
    title_font_widget='Text_Title',
    main_menu_path='/Game/HUD/MainMenu/WBP_MainMenuWidget',
    canvas_name='CanvasPanel_0',
    buttons_box_name='VB_MainButtons',
)

add_leaderboard_to_pause_menu(
    menu_path='/Game/HUD/InGameMenu/WBP_PauseMenu',
    panel_path='/Game/HUD/MainMenu/WBP_Leaderboard',
    button_class_path='/Game/HUD/WBP_GeoButton.WBP_GeoButton_C',
    buttons_box_name='MenuBox',
    panel_parent_name='MenuOverlayRoot',
    before_button_name='ReturnToMainMenuButton',
)

add_fight_timer_to_boss_bar(
    bar_path='/Game/HUD/ProgressBar/BP_BossHealthBar',
    bar_parent_class_path='/Script/GeoTrinityUI.GeoBossHealthBarWidget',
    health_bar_name='WBP_ProgressBar',
)
