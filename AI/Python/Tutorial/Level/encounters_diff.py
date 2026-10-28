"""Reads the tutorial scene as it is today and diffs it against what tutorial_encounters.py would write, changing nothing.
Run first, before editing tutorial_encounters.py or re-running it: every line is a hand edit to carry into the script
(or a script change about to overwrite the scene). Covers each room's steps (actions, triggers, conditions) and the spawn
points. Host: python AI/Python/Runtime/run_via_bridge.py AI/Python/Tutorial/Level/encounters_diff.py
Report: AI/Output/encounters_diff.txt ('-' = scene, '+' = script; empty section = identical).
"""
import difflib

import unreal

SCRIPT = "C:/GeoTrinity/AI/Python/Tutorial/Level/tutorial_encounters.py"
REPORT = "C:/GeoTrinity/AI/Output/encounters_diff.txt"
SCENE_REPORT = "C:/GeoTrinity/AI/Output/encounters_scene.txt"  # every room's steps as they are in the scene, whole
PROPS = ["Line", "bWaitForRespawn", "CounterName", "Delta", "AbilityTag", "bLocked", "Teleporter", "bOpen",
         "EnemyClass", "Setup", "Count", "RespawnDelay", "PurposeTag", "ArenaTag", "bKillOnStepExit", "Operation",
         "Title", "Max", "TimeLimitSeconds", "bShowMax", "Target", "Duration", "Volume", "Watched", "Delay",
         "bLooping", "Side", "Ratio", "Source", "Comparison", "Operand", "bForge", "Cat", "Path", "Speed",
         "StopDistance", "Settings", "Montage", "Actor", "bHidden", "bWaitForEnd", "AbilityClass", "EffectDataArray", "Params",
         "Team", "PatternData"]  # add a new action's properties here

ns = {"__name__": "tutorial_encounters"}
exec(compile(open(SCRIPT, encoding="utf-8").read(), SCRIPT, "exec"), ns)
ns["place"] = lambda label, *args, **kwargs: ns["find_actor"](label)  # reading only: never move a placed actor


def text(value):
    if value is None:
        return "None"
    if isinstance(value, (list, unreal.Array)):
        return "[%s]" % ", ".join(text(item) for item in value)
    if hasattr(value, "get_actor_label"):
        return "actor:" + value.get_actor_label()
    if hasattr(value, "export_text"):
        return value.export_text()
    if hasattr(value, "get_path_name"):
        return value.get_path_name()
    return "%.4f" % value if isinstance(value, float) else str(value)


def describe(obj):
    if obj is None:
        return "None"
    parts = []
    for name in PROPS:
        try:
            parts.append("%s=%s" % (name, text(obj.get_editor_property(name))))
        except Exception:
            pass
    return "%s(%s)" % (obj.get_class().get_name(), "; ".join(parts))


def dump_steps(steps):
    lines = []
    for step in steps:
        lines.append("STEP %s" % step.get_editor_property("Name"))
        for label, key in (("ENTER", "OnEnter"), ("EXIT", "OnExit")):
            lines += ["  %s %s" % (label, describe(action)) for action in step.get_editor_property(key)]
        for rule in step.get_editor_property("Rules"):
            lines.append("  RULE trigger=%s cond=%s repeat=%s completes=%s after=%s" % (
                describe(rule.get_editor_property("Trigger")), describe(rule.get_editor_property("Condition")),
                rule.get_editor_property("bRepeat"), rule.get_editor_property("bCompletesStep"),
                rule.get_editor_property("StepAfter")))
            lines += ["    DO %s" % describe(action) for action in rule.get_editor_property("Actions")]
    return lines


def point_line(label, location, tags):
    return "%s at (%.1f, %.1f, %.1f) %s" % (label, location[0], location[1], location[2], tags)


def scene_points(labels):
    lines = []
    for actor in ns["actors"]():
        label = actor.get_actor_label()
        if label in labels or (label.startswith("Tut_") and "Spawn" in label or label.startswith("Tut_") and any(
                kind in label for kind in ("Chaser", "Shooter", "Mover"))):
            loc = actor.get_actor_location()
            lines.append(point_line(label, (loc.x, loc.y, loc.z), actor.get_editor_property("GameplayTags").export_text()))
    return sorted(lines)


def script_points():
    expected = {}

    def record(label, location, purpose, room):
        tags = unreal.GameplayTagContainer()
        tags.import_text('(GameplayTags=((TagName="%s"),(TagName="Arena.Tutorial.%s")))' % (purpose, room))
        expected[label] = point_line(label, location, tags.export_text())

    ns["point"] = record
    ns["remove_actor"] = lambda label: None
    for build in ("build_shoot_room", "build_deploy_room", "build_special_room", "build_dummies_room",
                  "build_zone_points", "build_miniboss_points"):
        ns[build]()
    return sorted(expected.values()), set(expected)


def section(title, scene, script):
    delta = [line for line in difflib.unified_diff(scene, script, lineterm="", n=0) if line[:3] not in ("---", "+++", "@@ ")]
    return ["=== %s: %s" % (title, "identical" if not delta else "%d differing lines" % len(delta))] + delta


script_lines, labels = script_points()
report = section("spawn points", scene_points(labels), script_lines)
full_scene = []
for room in ns["ROOMS"]:
    builder = ns["Builder"](unreal.find_object(None, "/Engine/Transient"))
    script_steps = ns["room_steps"](builder, room)
    encounter = ns["find_actor"]("Tut_%s_Encounter" % room)
    scene_steps = encounter.get_editor_property("Steps") if encounter is not None else []
    report += section(room, dump_steps(scene_steps), dump_steps(script_steps))
    full_scene += ["##### " + room] + dump_steps(scene_steps)

open(REPORT, "w", encoding="utf-8").write("\n".join(report))
open(SCENE_REPORT, "w", encoding="utf-8").write("\n".join(full_scene))
