"""
Check of the tutorial's exit in one process, the way a player runs it (listen host, no PIE client): travel with the URL
UGeoGameInstance::LeaveTutorial builds and report where the player spawns. No mouse, no focus.
Usage: UnrealEditor-Win64-DebugGame.exe GeoTrinity.uproject /Game/Maps/Tutorial?listen -game -nullrhi -unattended
       "-ExecCmds=py C:/GeoTrinity/AI/Python/Tutorial/Runtime/pie_leave_tutorial.py"
Result in AI/Output/pie_leave_tutorial.txt; the game quits itself.
"""
import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/pie_leave_tutorial.txt"
TRAVEL_COMMAND = "open /Game/Maps/DraftMap?listen#TrainingRoom"
TUTORIAL_WORLD = "/Game/Maps/Tutorial.Tutorial"
TRAINING_WORLD = "/Game/Maps/DraftMap.DraftMap"
TRAVEL_AFTER_SECONDS = 8.0
REPORT_AFTER_SECONDS = 20.0

lines = []
state = {"elapsed": 0.0, "traveled": False, "handle": None}


def pawn_report(world):
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = controller.get_controlled_pawn() if controller else None
    return "world=%s pawn=%s at %s" % (world.get_path_name(), pawn.get_name() if pawn else None,
                                      pawn.get_actor_location() if pawn else None)


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as report:
        report.write("\n".join(lines))


def on_tick(delta):
    state["elapsed"] += delta
    # No editor subsystems in -game: the live world is found by its path.
    world = unreal.load_object(None, TRAINING_WORLD if state["traveled"] else TUTORIAL_WORLD)
    if world is None:
        return
    if not state["traveled"] and state["elapsed"] >= TRAVEL_AFTER_SECONDS:
        lines.append("before: " + pawn_report(world))
        unreal.SystemLibrary.execute_console_command(world, TRAVEL_COMMAND)
        state["traveled"] = True
    elif state["traveled"] and state["elapsed"] >= REPORT_AFTER_SECONDS:
        lines.append("after: " + pawn_report(world))
        flush()
        unreal.unregister_slate_post_tick_callback(state["handle"])
        unreal.SystemLibrary.execute_console_command(world, "quit")


state["handle"] = unreal.register_slate_post_tick_callback(on_tick)
