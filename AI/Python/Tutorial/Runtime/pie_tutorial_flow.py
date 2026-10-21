"""
Check of the tutorial's first rooms in one process, the way a player runs it (listen host, no PIE client, no mouse, no
focus): the script plays the player's part by moving its pawn, and kills dummies by dropping them under the world's KillZ.
Landing (movement locked while the Cat talks, unlocked at the WASD line, the lines run out, the exit pad opens; the 3D
Cat shows on its reassemble, then hides once it jumped into the pad) -> Dash
(arrival unlock, far platform, no fall) -> Shoot (first dummy, then the quest
cut by 12 kills). An ability page (unlock) pauses the solo game: the script closes it. Between rooms the pawn steps on the open exit pad (a real teleport); the entry pad never opens and the previous
exit closes on arrival. Game time runs at SLOMO times faster; Cat lines run on Slate time, so the script presses the
skip key (IA_InteractUI) every other frame, as a player mashing F. Each exit pad opening is the proof a room's encounter completed.
Usage: UnrealEditor-Win64-DebugGame.exe C:/GeoTrinity/GeoTrinity.uproject /Game/Maps/Tutorial?listen -game -RenderOffScreen -unattended
       -abslog=C:/GeoTrinity/AI/Output/tutorial_flow.log "-ExecCmds=py C:/GeoTrinity/AI/Python/Tutorial/Runtime/pie_tutorial_flow.py"
Result in AI/Output/pie_tutorial_flow.txt; the game quits itself (also after TIMEOUT_SECONDS).
"""
import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/pie_tutorial_flow.txt"
TUTORIAL_WORLD = "/Game/Maps/Tutorial.Tutorial"
SLOMO = 8
TIMEOUT_SECONDS = 240.0
KILLS_FOR_CUT = 12
SKIP_ACTION = "/Game/Input/InputActions/IA_InteractUI"

# Room geometry (tutorial_map.py): pitch = 2 * 700 + ROOM_GAP.
ROOM_PITCH = 2900.0
ROOM_HALF_X = 700.0
PLATFORM_X = 450.0
SHOOT_INDEX = 2

lines = []
state = {"elapsed": 0.0, "handle": None, "slomo": False, "kills": 0, "done": False, "room": 0, "stage": "Work",
         "last_log": 0.0, "pads": None, "work_start": 0.0, "locked": None, "cat_hidden": None}


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


def centre_x(room):
    return room * ROOM_PITCH


def pads_of(world):
    """Open state of every tutorial pad by actor name (Tut_Pad_<NN>_Exit / _Entry)."""
    return {p.get_name(): bool(p.get_editor_property("bOpen")) for p in actors_of(world, unreal.GeoTeleporter)
            if p.get_name().startswith("Tut_Pad_")}


def pad_location(world, name):
    return next(p for p in actors_of(world, unreal.GeoTeleporter) if p.get_name() == name).get_actor_location()


def dummies_of(world, room):
    """The living dummies of a room (the Special room has a placed one)."""
    return [e for e in actors_of(world, unreal.EnemyCharacter)
            if "BP_Dummy" in e.get_class().get_name()
            and abs(e.get_actor_location().x - centre_x(room)) < ROOM_HALF_X and e.get_actor_location().z > -100.0]


def drop_under_kill_z(character):
    location = character.get_actor_location()
    character.set_actor_location(unreal.Vector(location.x, location.y, -400.0), False, True)


def skip_cat_lines():
    """Press on even frames, release on odd ones: each press is a new Started, the Cat line skip."""
    state["skip_frame"] = state.get("skip_frame", 0) + 1
    value = unreal.Vector(1.0 if state["skip_frame"] % 2 else 0.0, 0.0, 0.0)
    action = unreal.load_asset(SKIP_ACTION)
    for subsystem in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem):
        if "Default__" not in subsystem.get_name():
            subsystem.inject_input_vector_for_action(action, value, [], [])


def watch_cat(world):
    cats = actors_of(world, unreal.GeoCat)
    hidden = bool(cats[0].get_editor_property("bHidden")) if cats else None
    if hidden != state["cat_hidden"]:
        log("cat hidden=%s" % hidden)
        state["cat_hidden"] = hidden


def finish(world, verdict):
    log(verdict)
    flush()
    unreal.unregister_slate_post_tick_callback(state["handle"])
    unreal.SystemLibrary.execute_console_command(world, "quit")
    state["done"] = True


def work(world, pawn, room):
    """The room's task, played by the pawn. True when the task is issued and the exit pad may open."""
    if room == 0:
        locked = pawn.get_controller().is_move_input_ignored()
        if locked != state["locked"]:
            log("movement locked=%s" % locked)
            state["locked"] = locked
    elif room == 1:
        pawn.set_actor_location(unreal.Vector(centre_x(1) + PLATFORM_X, 0.0, 60.0), False, True)
    elif room == SHOOT_INDEX and state["elapsed"] - state["work_start"] > 3.0:
        for dummy in dummies_of(world, room):
            if state["kills"] < KILLS_FOR_CUT:
                drop_under_kill_z(dummy)
                state["kills"] += 1
                log("killed dummy %d" % state["kills"])
    return True


def on_tick(delta):
    state["elapsed"] += delta
    world = unreal.load_object(None, TUTORIAL_WORLD)
    if world is None or state["done"]:
        return
    pawn = pawn_of(world)
    if pawn is None:
        return
    if not state["slomo"]:
        unreal.SystemLibrary.execute_console_command(world, "slomo %d" % SLOMO)
        state["slomo"] = True
    if unreal.GameplayStatics.is_game_paused(world):
        log("ability page paused the game: the player closes it")
        unreal.GameplayStatics.set_game_paused(world, False)
    skip_cat_lines()
    watch_cat(world)
    pads = pads_of(world)
    if pads != state["pads"]:
        log("pads open=%s" % sorted(n for n, o in pads.items() if o))
        state["pads"] = pads
    room = state["room"]
    exit_pad = "Tut_Pad_%02d_Exit" % room
    next_entry = "Tut_Pad_%02d_Entry" % (room + 1)
    if state["elapsed"] - state["last_log"] >= 10.0:
        state["last_log"] = state["elapsed"]
        cats = actors_of(world, unreal.GeoCat)
        log("room %d %s pawn %s cat %s enemies %s" % (room, state["stage"], pawn.get_actor_location(),
            cats[0].get_actor_location() if cats else None,
            [(e.get_class().get_name(), int(e.get_actor_location().x), int(e.get_actor_location().z))
             for e in actors_of(world, unreal.EnemyCharacter)]))
    if state["elapsed"] > TIMEOUT_SECONDS:
        finish(world, "TIMEOUT in room %d stage %s" % (room, state["stage"]))
    elif state["stage"] == "Work":
        work(world, pawn, room)
        if pads.get(exit_pad):
            log("room %d done: %s is open, %s still closed" % (room, exit_pad, next_entry))
            if pads.get(next_entry):
                finish(world, "FAILED: %s open before the player left" % next_entry)
            elif room == SHOOT_INDEX:
                finish(world, "PASSED: Shoot done after %d kills, its exit pad opened" % state["kills"])
            else:
                location = pad_location(world, exit_pad)
                pawn.set_actor_location(location, False, True)
                state["stage"] = "Teleport"
    elif state["stage"] == "Teleport":
        if pawn.get_actor_location().x > centre_x(room + 1) - ROOM_HALF_X:
            log("teleported to room %d at %s" % (room + 1, pawn.get_actor_location()))
            state["room"] = room + 1
            state["stage"] = "Arrive"
    elif state["stage"] == "Arrive":
        entry = "Tut_Pad_%02d_Entry" % room
        previous_exit = "Tut_Pad_%02d_Exit" % (room - 1)
        if pads.get(entry):
            finish(world, "FAILED: %s open on arrival" % entry)
        elif not pads.get(previous_exit):
            log("arrived: %s never opened, %s closed" % (entry, previous_exit))
            state["stage"] = "Work"
            state["work_start"] = state["elapsed"]


state["handle"] = unreal.register_slate_post_tick_callback(on_tick)
