"""
Edit a level's World Settings, GameMode override, and PlayerControllerClass from the editor.
Usage: run via MCP execute_script. The level must be loaded first (load_level), then save_current_level after.
"""
import unreal


def get_world_settings():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    return world.get_world_settings()


def set_world_setting(prop_name, value):
    ws = get_world_settings()
    ws.set_editor_property(prop_name, value)
    return ws.get_editor_property(prop_name)


def save_current_level():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()


# --- Example: point a menu map at its connect widget + GameMode override ---
def configure_menu_map():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level("/Game/Maps/MainMenuMap")

    # World Settings property (a UPROPERTY on the project's AWorldSettings subclass).
    wbp = unreal.load_asset("/Game/HUD/Menu/WBP_MainMenu")
    set_world_setting("connect_menu_widget_class", wbp.generated_class())

    # GameMode override lives on World Settings; PlayerControllerClass lives on the GameMode CDO.
    set_world_setting("default_game_mode", unreal.load_asset("/Game/Game/BP_MainMenuGameMode").generated_class())
    gm_cdo = unreal.get_default_object(unreal.load_asset("/Game/Game/BP_MainMenuGameMode").generated_class())
    gm_cdo.set_editor_property("player_controller_class",
                              unreal.load_asset("/Game/Characters/Playable/BP_GeoPlayerController").generated_class())
    unreal.EditorAssetLibrary.save_loaded_asset(unreal.load_asset("/Game/Game/BP_MainMenuGameMode"))

    save_current_level()
    unreal.log("level_settings.py: menu map configured.")


configure_menu_map()
