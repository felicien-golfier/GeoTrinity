"""
Check of the tutorial's jump start and a room-to-room teleport in one process (listen host, no PIE client, no mouse, no focus): the
game starts in the room named by the URL's portal (`Tutorial?listen#Zones`), the script plays that room's part and steps on
the final exit pad, then reports where the player lands. Only the last room is played: the pawn stands on the Shield then the Heal zone point,
stays in the last until the lethal wave kills it, then waits just past the safe-spot waves' reach (they hit by centre
only), so it survives all three; Deploy / Special need a heal and are left to the user. Cat lines are skipped as a player
mashing F would.
Usage: UnrealEditor-Win64-DebugGame.exe C:/GeoTrinity/GeoTrinity.uproject "/Game/Maps/Tutorial?listen#Zones" -game -RenderOffScreen -unattended
       -abslog=C:/GeoTrinity/AI/Output/tutorial_jump.log "-ExecCmds=py C:/GeoTrinity/AI/Python/Tutorial/Runtime/pie_tutorial_jump.py"
Result in AI/Output/pie_tutorial_jump.txt; the game quits itself (also after TIMEOUT_SECONDS).
"""
import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/pie_tutorial_jump.txt"
TUTORIAL_WORLD = "/Game/Maps/Tutorial.Tutorial"
SKIP_ACTION = "/Game/Input/InputActions/IA_InteractUI"
SLOMO = 8
TIMEOUT_SECONDS = 180.0
ROOM_PITCH = 2900.0   # tutorial_map.py: 2 * 700 + ROOM_GAP
LAST_ROOM = 6         # Zones, the room played
NEXT_ROOM_HALF_X = 700.0   # tutorial_map.py room half size
EXIT_PAD = "Tut_Pad_06_Exit"
ZONE_POINTS = ["Tut_Zones_ZonePoint_Shield", "Tut_Zones_ZonePoint_Heal"]  # the zones born after the Damage zone has grown
GROW_WAIT = 14.0      # CT_Tutorial ColorZoneGrowSeconds (8) plus the intro lines
ZONE_SECONDS = 2.0    # on each point
ENTRANCE_REACH_X = -300.0   # from the room's centre: a pawn behind it has respawned at the entrance
SAFE_STRIP_X = 662.0  # from the room's centre: past the waves' travel (tutorial_waves.py), which hit by centre only

lines = []
state = {"elapsed": 0.0, "handle": None, "slomo": False, "stage": "Start", "room": None, "skip_frame": 0,
         "pads": None, "locked": None, "zone": -1, "died": False}


def log(text):
    lines.append("[%6.1fs] %s" % (state["elapsed"], text))


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as report:
        report.write("\n".join(lines))


def actors_of(world, cls):
    return list(unreal.GameplayStatics.get_all_actors_of_class(world, cls))


def pawn_of(world):
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    return controller.get_controlled_pawn() if controller else None


def finish(world, verdict):
    log(verdict)
    flush()
    unreal.unregister_slate_post_tick_callback(state["handle"])
    unreal.SystemLibrary.execute_console_command(world, "quit")
    state["stage"] = "Done"


def skip_cat_lines():
    state["skip_frame"] += 1
    value = unreal.Vector(1.0 if state["skip_frame"] % 2 else 0.0, 0.0, 0.0)
    action = unreal.load_asset(SKIP_ACTION)
    for subsystem in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem):
        if "Default__" not in subsystem.get_name():
            subsystem.inject_input_vector_for_action(action, value, [], [])


def pad_location(world, name):
    return next(p for p in actors_of(world, unreal.GeoTeleporter) if p.get_name() == name).get_actor_location()


def pads_of(world):
    return {p.get_name(): bool(p.get_editor_property("bOpen")) for p in actors_of(world, unreal.GeoTeleporter)
            if p.get_name().startswith("Tut_Pad_")}


def play_zones(world, pawn):
    """The room's task: the Shield then the Heal point in turn, after the growth, the last one until the lethal wave kills the pawn, then the safe strip."""
    centre = LAST_ROOM * ROOM_PITCH
    index = int((state["elapsed"] - state["work_start"] - GROW_WAIT) / ZONE_SECONDS)
    if 0 <= index < len(ZONE_POINTS) and index != state["zone"]:
        volume = next(a for a in actors_of(world, unreal.Actor) if a.get_actor_label() == ZONE_POINTS[index])
        pawn.set_actor_location(volume.get_actor_location() + unreal.Vector(0.0, 0.0, 60.0), False, True)
        log("pawn on %s" % ZONE_POINTS[index])
        state["zone"] = index
    elif index >= len(ZONE_POINTS) and not state["died"] and pawn.get_actor_location().x < centre + ENTRANCE_REACH_X:
        log("pawn died to the lethal wave, respawned at %s" % pawn.get_actor_location())
        pawn.set_actor_location(unreal.Vector(centre + SAFE_STRIP_X, 0.0, 60.0), False, True)
        state["died"] = True


def on_tick(delta):
    state["elapsed"] += delta
    if state["stage"] == "Done":
        return
    world = unreal.load_object(None, TUTORIAL_WORLD)
    if world is None:
        return
    pawn = pawn_of(world)
    if state["elapsed"] > TIMEOUT_SECONDS:
        finish(world, "TIMEOUT in stage %s, pawn at %s" % (state["stage"], pawn.get_actor_location() if pawn else None))
        return
    if pawn is None:
        return
    if not state["slomo"]:
        unreal.SystemLibrary.execute_console_command(world, "slomo %d" % SLOMO)
        state["slomo"] = True
    if unreal.GameplayStatics.is_game_paused(world):
        log("ability page paused the game: the player closes it")
        unreal.GameplayStatics.set_game_paused(world, False)
    skip_cat_lines()
    pads = pads_of(world)
    if pads != state["pads"]:
        log("pads open=%s" % sorted(n for n, o in pads.items() if o))
        state["pads"] = pads
    locked = pawn.get_controller().is_move_input_ignored()
    if locked != state["locked"]:
        log("movement locked=%s" % locked)
        state["locked"] = locked
    if state["stage"] == "Start":
        state["room"] = int(round(pawn.get_actor_location().x / ROOM_PITCH))
        log("jump start: pawn at %s, room %d" % (pawn.get_actor_location(), state["room"]))
        if state["room"] != LAST_ROOM:
            finish(world, "FAILED: expected the Zones room (%d), got %d" % (LAST_ROOM, state["room"]))
            return
        state["stage"] = "Work"
        state["work_start"] = state["elapsed"]
    elif state["stage"] == "Work":
        play_zones(world, pawn)
        if pads.get(EXIT_PAD):
            log("room done: %s is open, stepping on it" % EXIT_PAD)
            pawn.set_actor_location(pad_location(world, EXIT_PAD), False, True)
            state["stage"] = "Teleport"
    elif state["stage"] == "Teleport":
        next_centre = (LAST_ROOM + 1) * ROOM_PITCH
        if pawn.get_actor_location().x > next_centre - NEXT_ROOM_HALF_X:
            finish(world, "PASSED: teleported to the Miniboss room at %s" % pawn.get_actor_location())


state["handle"] = unreal.register_slate_post_tick_callback(on_tick)
