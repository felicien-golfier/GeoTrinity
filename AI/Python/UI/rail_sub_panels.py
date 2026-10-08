"""
Dress every menu sub-panel in the Rail look: texts become UGeoText in their role, inputs their themed class wrapped in a
field frame, background images a panel frame. Places the main menu's sub-panels and gives the pause menu its title and
scrim. Look values live in DA_UITheme and the /Game/HUD/Style frame styles; this only sets structure.
Usage: run via MCP execute_script, after ui_theme.py, rail_style.py and rail_main_menu.py. Rebuilds whole widgets, so it
refuses an existing asset (asset_guard.legacy_rebuild) until converted to asset_guard.write.
"""
import unreal

UTIL = unreal.GeoWidgetBuilderUtil.get_default_object()
RAIL_STYLE_PATH = unreal.Paths.project_dir() + "AI/Python/UI/rail_style.py"
RAIL = {}
exec(compile(open(RAIL_STYLE_PATH, encoding="utf-8").read(), RAIL_STYLE_PATH, "exec"), RAIL)

ROLE = unreal.GeoTextRole
FIELD_PADDING = unreal.Margin(2, 2, 2, 2)
PANEL_PADDING = unreal.Margin(40, 32, 40, 32)
LIST_MARGIN = 64
SMALL_PANEL_WIDTH = 560


def fill_box_children(wbp, box_name):
    """Every child of the box spans its width, so a column of buttons and fields lines up."""
    for child in UTIL.find_widget(wbp, box_name).get_all_children():
        child.get_editor_property("slot").set_editor_property("horizontal_alignment",
                                                               unreal.HorizontalAlignment.H_ALIGN_FILL)


def theme_texts(wbp, roles):
    for name, role in roles.items():
        RAIL["theme_widget"](wbp, name, role)


def theme_fields(wbp, names):
    """Each input in its themed class, inside a field frame that lights up while it is hovered or focused."""
    for name in names:
        RAIL["theme_widget"](wbp, name)
        RAIL["wrap_in_frame"](wbp, name, name + "Frame", "DA_Frame_Field", FIELD_PADDING)


def replace_background(wbp, image_name, frame_name, style_name="DA_Frame_Panel"):
    """The image a panel was drawn on becomes a panel frame in the same place."""
    if UTIL.find_widget(wbp, image_name):
        overlay = UTIL.find_widget(wbp, image_name).get_parent().get_name()
        UTIL.remove_widget(wbp, image_name)
        RAIL["add_backdrop_frame"](wbp, overlay, frame_name, style_name)


def finish(wbp):
    UTIL.commit_tree(wbp)
    return RAIL["reload_blueprint"](wbp)


def dress_leaderboard():
    wbp = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/MainMenu/WBP_Leaderboard")
    theme_texts(wbp, {"TitleText": ROLE.HEADING})
    finish(wbp)


def dress_browse_servers():
    wbp = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/MainMenu/WBP_BrowseServers")
    theme_texts(wbp, {"TextBlock": ROLE.LABEL, "TextBlock_162": ROLE.LABEL})
    theme_fields(wbp, ["SearchInput", "LanguageComboBox"])
    RAIL["theme_widget"](wbp, "SearchProgressBar")
    if not UTIL.find_widget(wbp, "TitleText"):
        UTIL.construct_widget_in_tree(wbp, unreal.GeoText, "TitleText", True)
        title_slot = UTIL.attach_widget(wbp, "HorizontalBox_152", "TitleText", 0)
        title_slot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
        title_slot.set_editor_property("padding", unreal.Margin(0, 0, 32, 0))
    title = UTIL.find_widget(wbp, "TitleText")
    title.set_editor_property("text", unreal.Text("Join server"))
    title.set_editor_property("role", ROLE.HEADING)
    finish(wbp)


def dress_create_server():
    wbp = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/MainMenu/WBP_CreateServerWidget")
    replace_background(wbp, "Image_MenuBackground", "PanelFrame")
    theme_texts(wbp, {"Text_Title": ROLE.HEADING, "Text_ServerNameTitle": ROLE.LABEL, "Text_MapTitle": ROLE.LABEL,
                      "Text_SlotsTitle": ROLE.LABEL, "Text_LanguageTitle": ROLE.LABEL,
                      "Text_PricavyTitle": ROLE.LABEL})
    theme_fields(wbp, ["ServerNameInput", "MapComboBox", "SlotsComboBox", "LanguageComboBox", "PrivacyComboBox"])
    UTIL.find_widget(wbp, "VerticalBox_77").get_editor_property("slot").set_editor_property("padding", PANEL_PADDING)
    fill_box_children(wbp, "VerticalBox_77")
    UTIL.find_widget(wbp, "Root").set_min_desired_width(SMALL_PANEL_WIDTH)
    UTIL.find_widget(wbp, "Text_Title").set_editor_property("justification", unreal.TextJustify.CENTER)
    finish(wbp)


def dress_local_connect():
    wbp = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/MainMenu/WBP_LocalConnect")
    if not UTIL.find_widget(wbp, "PanelFrame"):
        RAIL["add_backdrop_frame"](wbp, "Root", "PanelFrame", "DA_Frame_Panel")
    if not UTIL.find_widget(wbp, "MenuWidth"):
        UTIL.construct_widget_in_tree(wbp, unreal.SizeBox, "MenuWidth", True)
        UTIL.attach_widget(wbp, "Root", "MenuWidth")
        UTIL.attach_widget(wbp, "MenuWidth", "Menu")
    UTIL.find_widget(wbp, "MenuWidth").set_min_desired_width(SMALL_PANEL_WIDTH)
    UTIL.find_widget(wbp, "MenuWidth").get_editor_property("slot").set_editor_property("padding", PANEL_PADDING)
    theme_texts(wbp, {"LocalIPText": ROLE.MONO})
    theme_fields(wbp, ["IPInput"])
    fill_box_children(wbp, "Menu")
    for child in UTIL.find_widget(wbp, "Menu").get_all_children():
        child.get_editor_property("slot").set_editor_property("padding", unreal.Margin(0, 8, 0, 8))
    UTIL.find_widget(wbp, "LocalIPText").set_editor_property("justification", unreal.TextJustify.CENTER)
    UTIL.find_widget(wbp, "IPInput").set_editor_property("justification", unreal.TextJustify.CENTER)
    finish(wbp)


def dress_pause_menu():
    """Panel frame for the plate, a scrim dimming the fight behind, and a PAUSED title as the menu's decor."""
    wbp = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/InGameMenu/WBP_PauseMenu")
    replace_background(wbp, "Image_Background", "PanelFrame")
    if not UTIL.find_widget(wbp, "Scrim"):
        RAIL["add_backdrop_frame"](wbp, "MenuOverlayRoot", "Scrim", "DA_Frame_Scrim")
    if not UTIL.find_widget(wbp, "MenuDecor"):
        UTIL.construct_widget_in_tree(wbp, unreal.GeoText, "MenuDecor", True)
        decor_slot = UTIL.attach_widget(wbp, "MenuBox", "MenuDecor", 0)
        decor_slot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_CENTER)
        decor_slot.set_editor_property("padding", unreal.Margin(0, 0, 0, 24))
    title = UTIL.find_widget(wbp, "MenuDecor")
    title.set_editor_property("text", unreal.Text("Paused"))
    title.set_editor_property("role", ROLE.HEADING)
    title.set_editor_property("visibility", unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    UTIL.find_widget(wbp, "MenuBox").get_editor_property("slot").set_editor_property("padding", PANEL_PADDING)
    finish(wbp)


def dress_settings():
    sound = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/InGameMenu/WBP_SoundSettings")
    theme_texts(sound, {"SoundLabel": ROLE.HEADING, "GeneralVolumeLabel": ROLE.BODY, "EffectsVolumeLabel": ROLE.BODY,
                        "MusicVolumeLabel": ROLE.BODY, "InterfaceVolumeLabel": ROLE.BODY})
    for slider in ["GeneralVolumeSlider", "EffectsVolumeSlider", "MusicVolumeSlider", "InterfaceVolumeSlider"]:
        RAIL["theme_widget"](sound, slider)
    finish(sound)

    keys = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/InGameMenu/WBP_KeyBindings")
    theme_texts(keys, {"KeyBindingsLabel": ROLE.HEADING, "SecondPlayerGamepadLabel": ROLE.BODY})
    RAIL["theme_widget"](keys, "SecondPlayerGamepadCheckBox")
    RAIL["theme_widget"](keys, "KeyBindingsList")
    finish(keys)

    abilities = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/InGameMenu/WBP_AbilityDescriptions")
    replace_background(abilities, "BackgroundImage", "PanelFrame")
    RAIL["theme_widget"](abilities, "AbilityList")
    finish(abilities)


def place_main_menu_panels():
    """List panels fill the screen inside a margin; the player box belongs to the top level."""
    wbp = RAIL["asset_guard"].legacy_rebuild("/Game/HUD/MainMenu/WBP_MainMenuWidget")
    for name in ["BrowseServerWidget", "LeaderboardWidget"]:
        slot = UTIL.find_widget(wbp, name).get_editor_property("slot")
        slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0, 0), maximum=unreal.Vector2D(1, 1)))
        slot.set_alignment(unreal.Vector2D(0, 0))
        slot.set_offsets(unreal.Margin(LIST_MARGIN, LIST_MARGIN, LIST_MARGIN, LIST_MARGIN))
        slot.set_auto_size(False)
    if UTIL.find_widget(wbp, "PlayerBox").get_parent().get_name() != "MenuDecor":
        slot_values = RAIL["read_properties"](UTIL.find_widget(wbp, "PlayerBox").get_editor_property("slot"),
                                              RAIL["CARRIED_SLOT_PROPERTIES"])
        RAIL["write_properties"](UTIL.attach_widget(wbp, "MenuDecor", "PlayerBox"), slot_values)
    finish(wbp)


if __name__ == "__main__":
    dress_leaderboard()
    dress_browse_servers()
    dress_create_server()
    dress_local_connect()
    dress_pause_menu()
    dress_settings()
    place_main_menu_panels()
