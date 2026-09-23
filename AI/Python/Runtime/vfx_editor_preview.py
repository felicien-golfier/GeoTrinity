"""Sets up the editor world so the level viewport can judge a visual change without PIE: preview systems, parameter
collection values, the camera.

Every function takes its assets and placement as arguments; the example call at the bottom is the only place a
path appears. Run through MCP execute_script against the editor world, and read REPORT back — the script tool
returns nothing a script prints.

Stepping a preview takes one call per step: a loop inside a single run holds the game thread, so the world
never ticks between iterations and nothing moves.

Preview actors are scenery the level does not own; clear() removes them, but spawning and destroying them
leaves the map dirty either way.

Reference: AI/MCP/MCP_Preview.md.
"""

import json
import os
import traceback

import unreal

LABEL = "GeoVFXPreview"
REPORT = "%sGeoTrinity_VFXPreview.txt" % unreal.Paths.project_saved_dir()
CAMERA = "%sGeoTrinity_PreviewCamera.json" % unreal.Paths.project_saved_dir()

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


def remember_camera(path=CAMERA):
    """Saves the viewport camera before a preview moves it; a second call keeps the first, which is the user's."""
    location, rotation = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_level_viewport_camera_info()
    if os.path.exists(path):
        out.append("camera already remembered in %s" % path)
    else:
        with open(path, "w") as f:
            json.dump({"location": [location.x, location.y, location.z],
                       "rotation": [rotation.roll, rotation.pitch, rotation.yaw]}, f)
        out.append("remembered camera %s %s" % (location, rotation))


def restore_camera(path=CAMERA):
    """Puts the viewport camera back where remember_camera found it, and forgets it."""
    with open(path) as f:
        camera = json.load(f)
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
        unreal.Vector(*camera["location"]), unreal.Rotator(*camera["rotation"]))
    os.remove(path)
    out.append("restored camera %s" % camera)


def set_collection_values(collection_path, vectors=None, scalars=None):
    """Writes parameter collection values in the editor world, where the materials reading them preview them."""
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    collection = unreal.load_asset(collection_path)
    for name, (r, g, b, a) in (vectors or {}).items():
        unreal.MaterialLibrary.set_vector_parameter_value(world, collection, name, unreal.LinearColor(r, g, b, a))
    for name, value in (scalars or {}).items():
        unreal.MaterialLibrary.set_scalar_parameter_value(world, collection, name, value)
    unreal.EditorLevelLibrary.editor_invalidate_viewports()
    out.append("collection %s: %s %s" % (collection_path, vectors or {}, scalars or {}))


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
        remember_camera()
        focus(0, len(SYSTEMS), ORIGIN, height=250.0)
        # A material reading a collection: one ring of the background pulse, (OriginX, OriginY, Radius, Intensity).
        set_collection_values("/Game/Art/VFX/Background/MPC_BackgroundPulse",
                              vectors={"PulseSource_00": (ORIGIN.x, ORIGIN.y, 500.0, 1.0)})
    except Exception:
        out.append("FAILED\n" + traceback.format_exc())
    report()
