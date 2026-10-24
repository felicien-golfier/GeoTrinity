"""
Check of the tutorial's Miniboss room in one process (listen host, no PIE client, no mouse, no focus): the game starts in
the room named by the URL's portal (`Tutorial?listen#Miniboss`), the player is invincible (`Geo.PlayerInvincible`), and the
script watches the fight: the boss walking between its 3 points, the summoned dummies landing, one of them killed (the
ally it turns into), the lateral wave. Then it kills the boss and steps on the final exit pad, and reports where the player
lands (Training Room start). Kills fall under the world's KillZ, never by damage: a real boss kill writes the user's gem
profile. Cat lines are skipped as a player mashing F would.
Usage: UnrealEditor-Win64-DebugGame.exe C:/GeoTrinity/GeoTrinity.uproject "/Game/Maps/Tutorial?listen#Miniboss" -game -RenderOffScreen -unattended
       -abslog=C:/GeoTrinity/AI/Output/tutorial_miniboss.log "-ExecCmds=py C:/GeoTrinity/AI/Python/Tutorial/Runtime/pie_tutorial_miniboss.py"
Result in AI/Output/pie_tutorial_miniboss.txt; the game quits itself (also after TIMEOUT_SECONDS).
"""
import math

import unreal

REPORT_PATH = "C:/GeoTrinity/AI/Output/pie_tutorial_miniboss.txt"
TUTORIAL_WORLD = "/Game/Maps/Tutorial.Tutorial"
TRAINING_WORLD = "/Game/Maps/DraftMap.DraftMap"
SKIP_ACTION = "/Game/Input/InputActions/IA_InteractUI"
SLOMO = 8
TIMEOUT_SECONDS = 300.0
ROOM_PITCH = 2900.0   # tutorial_map.py: 2 * 700 + ROOM_GAP
ROOM = 7              # Miniboss
BOSS_TAG = "TutorialMiniboss"  # the Miniboss arena's BossSetup.Tag: the boss is a BP_Dummy
SUMMONED_CLASS = "BP_TutorialSummonedDummy_C"
DUMMY_CLASS = "BP_Dummy_C"
ALLY_TAG = "TutorialMinibossAlly"   # tutorial_encounters.py MINIBOSS_ALLY_TAG
EXIT_PAD_LABEL = "Tut_Miniboss_PadExit"
EXIT_VOLUME_LABEL = "Tut_Miniboss_PadExitVolume"
POINT_LABELS = ["Tut_Miniboss_MinibossPoint_%d" % i for i in range(3)]
POINT_REACH = 120.0   # a boss this close to a point has reached it
WAVE_WAIT_SECONDS = 90.0   # real seconds after the ally appeared that the wave may take
TRAINING_START_X = -6000.0   # tutorial_map.py: TRAINING_START_LOCATION

lines = []
state = {"elapsed": 0.0, "handle": None, "slomo": False, "stage": "Start", "skip_frame": 0, "last_log": 0.0,
         "points_reached": set(), "zone_seen": False, "ally_seen": False, "wave_seen": False,
         "strips": None, "summoned_names": set(), "ally_time": 0.0}


def log(text):
    lines.append("[%6.1fs] %s" % (state["elapsed"], text))


def flush():
    with open(REPORT_PATH, "w", encoding="utf-8") as report:
        report.write("\n".join(lines))


def actors_of(world, cls):
    return list(unreal.GameplayStatics.get_all_actors_of_class(world, cls))


def labelled(world, cls, label):
    return next((a for a in actors_of(world, cls) if a.get_actor_label() == label), None)


def of_class(world, cls, class_name):
    return [a for a in actors_of(world, cls) if a.get_class().get_name() == class_name]


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


def drop_under_kill_z(character):
    location = character.get_actor_location()
    character.set_actor_location(unreal.Vector(location.x, location.y, -400.0), False, True)


def watch_fight(world, boss):
    """Boss position against the 3 points, summoned dummies, zones, allies and any new actor class (the wave's)."""
    location = boss.get_actor_location()
    for index, label in enumerate(POINT_LABELS):
        point = labelled(world, unreal.TargetPoint, label)
        where = point.get_actor_location() if point is not None else None
        if where is not None and math.hypot(where.x - location.x, where.y - location.y) < POINT_REACH \
                and index not in state["points_reached"]:
            state["points_reached"].add(index)
            log("boss reached point %d at %s" % (index, location))
    summoned = of_class(world, unreal.EnemyCharacter, SUMMONED_CLASS)
    for dummy in summoned:
        if dummy.get_name() not in state["summoned_names"]:
            state["summoned_names"].add(dummy.get_name())
            log("summoned dummy %d landed at %s" % (len(state["summoned_names"]), dummy.get_actor_location()))
    if not state["zone_seen"] and actors_of(world, unreal.GeoEffectZone):
        state["zone_seen"] = True
        log("growing zone present")
    if not state["ally_seen"] and any(a.actor_has_tag(ALLY_TAG)
                                      for a in of_class(world, unreal.EnemyCharacter, DUMMY_CLASS)):
        state["ally_seen"] = True
        state["ally_time"] = state["elapsed"]
        log("ally dummy present")
    strips = len(boss.get_components_by_class(unreal.NiagaraComponent))
    if strips != state["strips"]:
        log("boss niagara components: %s -> %d" % (state["strips"], strips))
        state["wave_seen"] = state["wave_seen"] or (state["strips"] is not None and strips > state["strips"])
        state["strips"] = strips
    return summoned


def landed(training):
    pawn = pawn_of(training) if training else None
    if pawn is not None:
        location = pawn.get_actor_location()
        ok = abs(location.x - TRAINING_START_X) < 300.0
        problems = verdict()
        finish(training, "%s: landed at %s in %s (TrainingRoom start x = %.0f); fight problems: %s" % (
            "PASSED" if ok and not problems else "FAILED", location, training.get_path_name(), TRAINING_START_X,
            problems or "none"))
    return pawn is not None


def verdict():
    problems = []
    if len(state["points_reached"]) < 2:
        problems.append("boss reached %d of 3 points" % len(state["points_reached"]))
    if not state["summoned_names"]:
        problems.append("no summoned dummy landed")
    if not state["zone_seen"]:
        problems.append("no growing zone")
    if not state["ally_seen"]:
        problems.append("no ally after a summoned dummy died")
    if not state["wave_seen"]:
        problems.append("no wave strips seen on the boss (a Niagara component added to it)")
    return problems


def on_tick(delta):
    state["elapsed"] += delta
    if state["stage"] == "Done":
        return
    if state["stage"] == "Exit" and landed(unreal.load_object(None, TRAINING_WORLD)):
        return
    world = unreal.load_object(None, TUTORIAL_WORLD)
    pawn = pawn_of(world) if world else None
    if pawn is None:
        return
    if state["elapsed"] > TIMEOUT_SECONDS:
        finish(world, "TIMEOUT in stage %s; %s" % (state["stage"], verdict()))
        return
    if not state["slomo"]:
        unreal.SystemLibrary.execute_console_command(world, "slomo %d" % SLOMO)
        unreal.SystemLibrary.execute_console_command(world, "Geo.PlayerInvincible 1")
        state["slomo"] = True
    if unreal.GameplayStatics.is_game_paused(world):
        unreal.GameplayStatics.set_game_paused(world, False)
    skip_cat_lines()
    if state["stage"] == "Start":
        room = int(round(pawn.get_actor_location().x / ROOM_PITCH))
        log("jump start: pawn at %s, room %d" % (pawn.get_actor_location(), room))
        if room != ROOM:
            finish(world, "FAILED: expected the Miniboss room (%d), got %d" % (ROOM, room))
        else:
            state["stage"] = "Fight"
    elif state["stage"] == "Fight":
        boss = next((a for a in of_class(world, unreal.EnemyCharacter, DUMMY_CLASS) if a.actor_has_tag(BOSS_TAG)), None)
        if boss is None:
            if state["elapsed"] - state["last_log"] >= 5.0:
                state["last_log"] = state["elapsed"]
                log("no boss yet, pawn at %s" % pawn.get_actor_location())
        else:
            summoned = watch_fight(world, boss)
            if state["elapsed"] - state["last_log"] >= 10.0:
                state["last_log"] = state["elapsed"]
                log("boss %s, %d summoned so far, wave %s" % (boss.get_actor_location(), len(state["summoned_names"]), state["wave_seen"]))
            if state["zone_seen"]:
                for dummy in summoned:
                    drop_under_kill_z(dummy)
                    log("summoned dummy dropped under KillZ")
            ally_done = state["ally_seen"] and (state["wave_seen"] or state["elapsed"] - state["ally_time"] > WAVE_WAIT_SECONDS)
            if ally_done:
                drop_under_kill_z(boss)
                log("boss dropped under KillZ")
                state["stage"] = "Kill"
    elif state["stage"] == "Kill":
        pad = labelled(world, unreal.GeoTeleporter, EXIT_PAD_LABEL)
        if pad is None:
            finish(world, "FAILED: %s not in the level" % EXIT_PAD_LABEL)
        elif pad.get_editor_property("bOpen"):
            log("boss dead, %s open" % EXIT_PAD_LABEL)
            volume = labelled(world, unreal.TriggerBox, EXIT_VOLUME_LABEL)
            if volume is None:
                finish(world, "FAILED: %s not in the level" % EXIT_VOLUME_LABEL)
            else:
                pawn.set_actor_location(volume.get_actor_location() + unreal.Vector(0.0, 0.0, 60.0), False, True)
                state["stage"] = "Exit"


state["handle"] = unreal.register_slate_post_tick_callback(on_tick)
