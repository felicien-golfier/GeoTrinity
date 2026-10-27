"""Read-only: dumps the whole Tutorial level as it is in the editor, every actor, to AI/Output/tutorial_scene.txt.
Per actor: class, label, outliner folder, parent, tags, transform, hidden, components (transform, box extents) and every
editable property that differs from the class default. Companion of encounters_diff.py (rooms' steps); together they hold
all the scene. Host: python AI/Python/Runtime/run_via_bridge.py AI/Python/Tutorial/Level/scene_dump.py
Compare to the previous dump (git) or to what the scripts place; the scene is the truth (Tutorial/CLAUDE.md).
"""
import re

import unreal

MAP = "/Game/Maps/Tutorial"
REPORT = "C:/GeoTrinity/AI/Output/tutorial_scene.txt"
SKIP = {"actor_label", "tags", "root_component", "folder_path", "replicates", "actor_guid", "body_instance"}  # own line, or noise


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
    if isinstance(value, float):
        return "%.3f" % value
    return re.sub(r" \(0x[0-9A-Fa-f]+\)", "", str(value))  # a struct or object repr carries its address


def changed_properties(obj):
    """Editable properties whose value differs from the class default."""
    default = unreal.get_default_object(obj.get_class())
    out = []
    for name in sorted(dir(obj)):
        if name.startswith("_") or name in SKIP:
            continue
        try:
            if callable(getattr(obj, name)):
                continue
            value, base = text(obj.get_editor_property(name)), text(default.get_editor_property(name))
        except Exception:
            continue
        if value != base and ":PersistentLevel." not in value:  # a component of the actor itself is listed as a comp
            out.append("%s=%s" % (name, value))
    return out


EXTRA = [  # properties dir() does not list: (class, names), always printed
    (unreal.GeoArena, ["boss_class", "boss_setup", "arena_tag", "boss_type", "boss_loot", "records_leaderboard"]),
    (unreal.GeoArenaVolume, ["arena_tag"]),
    (unreal.GeoEncounter, ["jump_start", "start_on_begin_play"]),
    (unreal.GeoWorldSettings, ["default_game_mode", "starting_class_override", "locked_ability_tags"]),
]


def extra_properties(actor):
    out = []
    for cls, names in EXTRA:
        if isinstance(actor, cls):
            for name in names:
                try:
                    out.append("%s=%s" % (name, text(actor.get_editor_property(name))))
                except Exception as error:
                    out.append("%s=<%s>" % (name, error))
    return out


def transform(comp):
    t = comp.get_world_transform() if hasattr(comp, "get_world_transform") else None
    if t is None:
        return ""
    return "loc=%s rot=%s scale=%s" % (text(t.translation), text(t.rotation.rotator()), text(t.scale3d))


def dump_actor(actor):
    lines = ["ACTOR %s [%s] folder=%s" % (actor.get_actor_label(), actor.get_class().get_name(),
                                          actor.get_folder_path())]
    parent = actor.get_attach_parent_actor()
    lines.append("  parent=%s tags=%s hidden=%s" % (parent.get_actor_label() if parent else "None",
                                                     [str(t) for t in actor.tags], actor.is_temporarily_hidden_in_editor()))
    root = actor.get_editor_property("root_component")
    if root:
        lines.append("  root " + transform(root))
        if isinstance(root, unreal.BoxComponent):
            lines.append("  root box_extent=%s" % text(root.get_editor_property("box_extent")))
    lines += ["  prop " + p for p in changed_properties(actor) + extra_properties(actor)]
    for comp in actor.get_components_by_class(unreal.SceneComponent):
        if comp == root:
            continue
        extra = changed_properties(comp)
        if isinstance(comp, unreal.BoxComponent):
            extra.append("box_extent=%s" % text(comp.get_editor_property("box_extent")))
        lines.append("  comp %s [%s] %s %s" % (comp.get_name(), comp.get_class().get_name(), transform(comp),
                                               "; ".join(extra)))
    return lines


def run():
    if unreal.EditorLevelLibrary.get_editor_world().get_path_name().split(".")[0] != MAP:
        unreal.EditorLevelLibrary.load_level(MAP)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    actors.sort(key=lambda a: (a.get_folder_path().__str__(), a.get_actor_label()))
    lines = ["%d actors in %s" % (len(actors), MAP)]
    settings = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings()
    lines += ["WORLD SETTINGS"] + ["  prop " + p for p in extra_properties(settings)]
    for actor in actors:
        lines += dump_actor(actor)
    with open(REPORT, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    unreal.log("scene_dump: %d actors -> %s" % (len(actors), REPORT))


run()
