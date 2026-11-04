"""The map gate's tool (AI/Python/Level/CLAUDE.md, "Map gate"): tells whether a map changed since Claude's last script run.
Usage, map name without path:
    python AI/Python/Runtime/run_via_bridge.py AI/Python/Level/map_guard.py 8090 check Tutorial
    Editor closed: UnrealEditor-Win64-DebugGame-Cmd.exe GeoTrinity.uproject -run=pythonscript
                   "-script=AI/Python/Level/map_guard.py check Tutorial" -unattended -nullrhi -NoP4
check   save if dirty, hash; dump + diff only on a hash change. First report line: SAME, RESAVED, CHANGED or NO SNAPSHOT.
after   after the script ran: what moved against the dump before it.
commit  write the new snapshot + hash.
Report: AI/Output/map_guard.txt ('-' before, '+' map now). Snapshots: AI/Output/MapSnapshots/, not committed.
"""
import difflib
import hashlib
import os
import re
import subprocess
import sys

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SNAPSHOTS = PROJECT + "AI/Output/MapSnapshots/"
REPORT = PROJECT + "AI/Output/map_guard.txt"
ACTOR_NAME = re.compile(r"Begin Actor .*?Name=(\S+)")
ACTOR_CLASS = re.compile(r"Begin Actor Class=\S*\.(\w+)")
OBJECT_NAME = re.compile(r'Begin Object .*?Name="([^"]+)"')
OBJECT_REFERENCE = re.compile(r"\"?/Script/[\w.]+'(\w+)'\"?")
LEVEL_PATH = re.compile(r"PersistentLevel\.(\w+)")
SPAWNED_ON_LEVEL_LOAD = ("AbstractNavData", "BuoyancyManager", "GameplayDebuggerPlayerManager",
                         "GeoZoneLayerActor")  # never in the saved map
DERIVED_LINE = re.compile(r"\| BodySetup_\d+ \||\| CachedBounds\(|^Actor \| ")  # the engine rebuilds these, nobody edits them
LOCALIZED_TEXT = re.compile(r'NSLOCTEXT\("[^"]*", "[^"]*", ')  # its keys are generated, per package


def package_of(map_name):
    return "/Game/Maps/" + map_name


def relative_map_file(map_name):
    return "Content/Maps/%s.umap" % map_name


def snapshot_file(map_name):
    return "%s%s.txt" % (SNAPSHOTS, map_name)


def hash_file(map_name):
    return "%s%s.hash" % (SNAPSHOTS, map_name)


def before_file(map_name):
    return "%s%s.before.txt" % (SNAPSHOTS, map_name)


def read_lines(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read().splitlines()


def write_lines(path, lines):
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines) + "\n")


def report(status, lines=()):
    write_lines(REPORT, [status] + list(lines))
    print(status)


def save_if_dirty(map_name):
    for package in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():
        if package.get_name() == package_of(map_name) and not unreal.EditorLoadingAndSavingUtils.save_packages([package], True):
            raise RuntimeError("%s would not save" % map_name)


def map_hash(map_name):
    with open(PROJECT + relative_map_file(map_name), "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()


def matches_git_head(map_name):
    """Clean against HEAD: the change came with a pull or a commit, not a local edit."""
    status = subprocess.run(["git", "status", "--porcelain", "--", relative_map_file(map_name)], cwd=PROJECT, text=True,
                            capture_output=True, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
    return not status.stdout.strip()


def open_map(map_name):
    """Always the level as opened: construction scripts rewrite values a plain asset load leaves as saved. The script the
    gate runs for opens this map anyway."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if world is None or world.get_outermost().get_name() != package_of(map_name):
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(package_of(map_name))
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def export_t3d(map_name):
    os.makedirs(SNAPSHOTS, exist_ok=True)
    path = "%s%s.t3d" % (SNAPSHOTS, map_name)
    task = unreal.AssetExportTask()
    task.object = open_map(map_name)
    task.filename = path
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError("%s: T3D export failed: %s" % (map_name, list(task.errors)))

    lines = read_lines(path)
    os.remove(path)
    return lines


def parse_actors(t3d):
    """Actors as {name, properties, objects: {name: lines}, instanced}, plus the lines outside any actor."""
    actors, outside = [], []
    actor, current_object = None, None
    for raw in t3d:
        line = raw.strip()
        if line.startswith("Begin Actor "):
            actor = {"name": ACTOR_NAME.search(line).group(1), "properties": [], "objects": {}, "instanced": set(),
                     "class": ACTOR_CLASS.search(line).group(1)}
        elif line == "End Actor":
            if actor["class"] not in SPAWNED_ON_LEVEL_LOAD:
                actors.append(actor)
            actor = None
        elif actor is None:
            outside.append(line)
        elif line.startswith("Begin Object "):
            current_object = OBJECT_NAME.search(line).group(1)
            if line.startswith("Begin Object Class=") and "Archetype=" not in line:
                actor["instanced"].add(current_object)
            elif not line.startswith("Begin Object Class="):
                actor["objects"][current_object] = []
        elif line == "End Object":
            current_object = None
        elif current_object is None:
            actor["properties"].append(line)
        elif current_object in actor["objects"]:
            actor["objects"][current_object].append(line)
    return actors, outside


def inline_instanced(line, actor, used):
    """Puts an instanced object's values where it is referenced: its generated number moves when a script rewrites it."""

    def expand(match):
        name = match.group(1)
        if name not in actor["instanced"] or name not in actor["objects"]:
            return match.group(0)
        used.add(name)
        values = [inline_instanced(value, actor, used) for value in actor["objects"][name]]
        return "%s{%s}" % (re.sub(r"_\d+$", "", name), "; ".join(values))

    return OBJECT_REFERENCE.sub(expand, line)


def normalized(t3d):
    """One sorted line per property, 'actor label | object | property', free of generated names."""
    actors, outside = parse_actors(t3d)
    labels = {}
    for actor in actors:
        for line in actor["properties"]:
            if line.startswith("ActorLabel="):
                labels[actor["name"]] = line.split("=", 1)[1].strip('"')

    out = ["MAP | " + line for line in outside if not line.startswith(("Begin Map", "End Map"))]
    for actor in actors:
        used = set()
        label = labels.get(actor["name"], actor["name"])
        lines = ["%s | %s" % (label, inline_instanced(line, actor, used)) for line in actor["properties"]]
        for name in sorted(actor["objects"]):
            if name not in actor["instanced"]:  # an instanced object no property holds is a script's leftover, never saved
                lines += ["%s | %s | %s" % (label, name, inline_instanced(value, actor, used)) for value in actor["objects"][name]]
        out += lines
    out = [LOCALIZED_TEXT.sub("TEXT(", line) for line in out if not DERIVED_LINE.search(line)]
    return sorted(LEVEL_PATH.sub(lambda match: "PersistentLevel." + labels.get(match.group(1), match.group(1)), line) for line in out)


def dump(map_name):
    return normalized(export_t3d(map_name))


def difference(before, now):
    return [line for line in difflib.unified_diff(before, now, lineterm="", n=0) if line[:3] not in ("---", "+++", "@@ ")]


def check(map_name):
    save_if_dirty(map_name)
    if os.path.exists(before_file(map_name)):
        os.remove(before_file(map_name))

    if not os.path.exists(hash_file(map_name)):
        report("NO SNAPSHOT %s: sync the scripts with the scene, then commit" % map_name)
    elif read_lines(hash_file(map_name))[0] == map_hash(map_name):
        report("SAME %s: run the script" % map_name)
    else:
        now = dump(map_name)
        delta = difference(read_lines(snapshot_file(map_name)), now)
        if delta:
            write_lines(before_file(map_name), now)
            origin = " (file matches git HEAD: came with a pull or a commit)" if matches_git_head(map_name) else ""
            report("CHANGED %s%s: %d lines to carry into the script" % (map_name, origin, len(delta)), delta)
        else:
            write_lines(hash_file(map_name), [map_hash(map_name)])
            report("RESAVED %s: no difference, run the script" % map_name)


def after(map_name):
    save_if_dirty(map_name)
    base = before_file(map_name) if os.path.exists(before_file(map_name)) else snapshot_file(map_name)
    delta = difference(read_lines(base), dump(map_name))
    report("AFTER %s: %d lines moved by the run" % (map_name, len(delta)), delta)


def commit(map_name):
    save_if_dirty(map_name)
    write_lines(snapshot_file(map_name), dump(map_name))
    write_lines(hash_file(map_name), [map_hash(map_name)])
    if os.path.exists(before_file(map_name)):
        os.remove(before_file(map_name))
    report("COMMITTED %s" % map_name)


if __name__ == "__main__":
    {"check": check, "after": after, "commit": commit}[sys.argv[1]](sys.argv[2])
