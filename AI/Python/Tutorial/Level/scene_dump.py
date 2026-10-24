"""Read-only: dumps the whole Tutorial level as it is in the editor, every actor, to AI/Output/tutorial_scene.txt.
Per actor: class, label, outliner folder, parent, tags, transform, hidden, components (transform, box extents) and every
editable property that differs from the class default. Companion of encounters_diff.py (rooms' steps); together they hold
all the scene. Host: python AI/Python/Runtime/run_via_bridge.py AI/Python/Tutorial/Level/scene_dump.py
Compare to the previous dump (git) or to what the scripts place; the scene is the truth (Tutorial/CLAUDE.md).
"""
import unreal

MAP = "/Game/Maps/Tutorial"
REPORT = "C:/GeoTrinity/AI/Output/tutorial_scene.txt"
SKIP = {"actor_label", "tags", "root_component", "folder_path", "replicates"}  # reported on their own line


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
    return "%.3f" % value if isinstance(value, float) else str(value)


def changed_properties(obj):
    """Editable properties whose value differs from the class default."""
    default = obj.get_class().get_default_object()
    out = []
    for name in sorted(dir(obj)):
        if name.startswith("_") or name in SKIP or callable(getattr(type(obj), name, None)):
            continue
        try:
            value, base = text(obj.get_editor_property(name)), text(default.get_editor_property(name))
        except Exception:
            continue
        if value != base:
            out.append("%s=%s" % (name, value))
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
    lines += ["  prop " + p for p in changed_properties(actor)]
    for comp in actor.get_components_by_class(unreal.SceneComponent):
        if comp == root:
            continue
        extra = changed_properties(comp)
        lines.append("  comp %s [%s] %s %s" % (comp.get_name(), comp.get_class().get_name(), transform(comp),
                                               "; ".join(extra)))
    return lines


def run():
    if unreal.EditorLevelLibrary.get_editor_world().get_path_name().split(".")[0] != MAP:
        unreal.EditorLevelLibrary.load_level(MAP)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    actors.sort(key=lambda a: (a.get_folder_path().__str__(), a.get_actor_label()))
    lines = ["%d actors in %s" % (len(actors), MAP)]
    for actor in actors:
        lines += dump_actor(actor)
    with open(REPORT, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    unreal.log("scene_dump: %d actors -> %s" % (len(actors), REPORT))


run()
