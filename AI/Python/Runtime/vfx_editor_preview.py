"""Places systems in the editor world so the level viewport can judge them without PIE.

Every function takes its assets and placement as arguments; the example call at the bottom is the only place a
path appears. Run through MCP execute_script against the editor world, and read REPORT back — the script tool
returns nothing a script prints.

Stepping a preview takes one call per step: a loop inside a single run holds the game thread, so the world
never ticks between iterations and nothing moves.

Preview actors are scenery the level does not own; clear() removes them, but spawning and destroying them
leaves the map dirty either way.

Reference: AI/MCP/MCP_Preview.md.
"""

import traceback

import unreal

LABEL = "GeoVFXPreview"
REPORT = "%sGeoTrinity_VFXPreview.txt" % unreal.Paths.project_saved_dir()

out = []


def report():
    open(REPORT, "w").write("\n".join(out))
    return "\n".join(out)


def previews(label=LABEL):
    return [actor for actor in unreal.EditorActorSubsystem().get_all_level_actors()
            if actor.get_actor_label().startswith(label)]


def clear(label=LABEL):
    """A re-run replaces the previous previews rather than stacking copies."""
    actors = unreal.EditorActorSubsystem()
    for actor in previews(label):
        out.append("destroyed %s" % actor.get_actor_label())
        actors.destroy_actor(actor)


def slot(index, count, origin, spacing):
    """Where the index'th preview sits: a row centred on origin, laid out along X."""
    return unreal.Vector(origin.x + spacing * (index - (count - 1) / 2.0), origin.y, origin.z)


def place(system_paths, origin, spacing=420.0, mesh_path="", label=LABEL):
    """Spawns each system in a row; a mesh path gives every one a skeletal mesh actor to attach to."""
    actors = unreal.EditorActorSubsystem()
    for index, system_path in enumerate(system_paths):
        location = slot(index, len(system_paths), origin, spacing)
        parent = None
        if mesh_path:
            parent = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, location, unreal.Rotator())
            parent.set_actor_label("%s_Mesh_%d" % (label, index))
            parent.skeletal_mesh_component.set_skeletal_mesh_asset(unreal.load_object(None, mesh_path))

        # Spawned from the asset, the way a drag into the level does it; setting the asset on an already
        # registered component leaves its renderers uninitialised in the editor world.
        actor = actors.spawn_actor_from_object(unreal.load_object(None, system_path), location,
                                               unreal.Rotator())
        actor.set_actor_label("%s_%s" % (label, system_path.rsplit("/", 1)[-1].split(".")[-1]))
        if parent:
            actor.attach_to_actor(parent, "", unreal.AttachmentRule.SNAP_TO_TARGET,
                                  unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.KEEP_WORLD,
                                  False)
        actor.niagara_component.activate(True)
        out.append("%-40s active=%s at %s" % (actor.get_actor_label(),
                                              actor.niagara_component.is_active(), location))


def focus(index, count, origin, spacing=420.0, height=250.0):
    """Looks straight down at one preview; height is also half the world width the frame then covers."""
    location = slot(index, count, origin, spacing)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    editor.set_level_viewport_camera_info(unreal.Vector(location.x, location.y, location.z + height),
                                          unreal.Rotator(0.0, -90.0, 0.0))
    unreal.EditorLevelLibrary.editor_invalidate_viewports()
    out.append("focused %d of %d, camera %s" % (index, count, editor.get_level_viewport_camera_info()))


def step(delta, label=LABEL):
    """Moves every preview once. Call once per step, or the world never ticks between them."""
    for actor in previews(label):
        actor.set_actor_location(actor.get_actor_location() + delta, False, False)
    out.append("stepped %d previews by %s" % (len(previews(label)), delta))


def particle_counts(system_filter="NS_*", enabled=True):
    """Overlays each live system's emitter and particle counts, which is what says whether it simulates."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    command = ("fx.Niagara.Debug.Hud Enabled=1 OverviewEnabled=1 SystemFilter=%s" % system_filter
               if enabled else "fx.Niagara.Debug.Hud Enabled=0")
    unreal.SystemLibrary.execute_console_command(world, command)
    out.append(command)


def render_while_unfocused(enabled):
    """Lets the editor keep drawing while another window has focus; restore it when the loop is done."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "Slate.bAllowThrottling %d" % (0 if enabled else 1))
    settings = unreal.load_object(None, "/Script/UnrealEd.Default__EditorPerformanceSettings")
    settings.set_editor_property("bThrottleCPUWhenNotForeground", not enabled)
    out.append("render while unfocused: %s" % enabled)


if __name__ == "__main__":
    FOLDER = "/Game/Art/VFX/Generic/Niagara"
    SYSTEMS = ["%s/%s.%s" % (FOLDER, name, name) for name in ("NS_ChargedTrail", "NS_ChargedHalo")]
    ORIGIN = unreal.Vector(0.0, 9000.0, 3000.0)
    try:
        clear()
        render_while_unfocused(True)
        place(SYSTEMS, ORIGIN)
        focus(0, len(SYSTEMS), ORIGIN, height=250.0)
    except Exception:
        out.append("FAILED\n" + traceback.format_exc())
    report()
