"""Places a Niagara system on a character mesh in the editor world, so the level viewport previews it without PIE.

The asset editor's own preview has no attach parent, so a mesh-sampled system shows empty there; only a real
skeletal mesh actor in a world resolves SkeletalMeshLocation.
"""

import unreal
import traceback

SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_Empower.NS_Empower"
MESH = "/Game/Characters/Meshes/Cone/SKM_Cone.SKM_Cone"
LABEL = "GeoVFXPreview"
LOCATION = unreal.Vector(0.0, 3000.0, 900.0)
CAMERA_HEIGHT = 800.0
REPORT = "%sGeoTrinity_VFXPreview.txt" % unreal.Paths.project_saved_dir()

out = []


def clear(world, actors):
    """Preview actors are transient scenery; a re-run replaces them rather than stacking copies."""
    for actor in actors:
        if actor.get_actor_label().startswith(LABEL):
            unreal.EditorActorSubsystem().destroy_actor(actor)
            out.append("destroyed %s" % actor.get_actor_label())


def build():
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor.get_editor_world()
    out.append("world %s" % world.get_path_name())
    actors = unreal.EditorActorSubsystem().get_all_level_actors()
    clear(world, actors)

    mesh_actor = unreal.EditorActorSubsystem().spawn_actor_from_class(
        unreal.SkeletalMeshActor, LOCATION, unreal.Rotator(0.0, 0.0, 0.0))
    mesh_actor.set_actor_label("%s_Mesh" % LABEL)
    mesh_actor.skeletal_mesh_component.set_skeletal_mesh_asset(unreal.load_object(None, MESH))
    out.append("mesh actor %s" % mesh_actor.get_actor_label())

    # Spawned from the asset, the way a drag into the level does it; setting the asset on an already
    # registered component leaves its renderers uninitialised in the editor world.
    vfx_actor = unreal.EditorActorSubsystem().spawn_actor_from_object(
        unreal.load_object(None, SYSTEM), LOCATION, unreal.Rotator(0.0, 0.0, 0.0))
    vfx_actor.set_actor_label("%s_System" % LABEL)
    vfx_actor.attach_to_actor(mesh_actor, "", unreal.AttachmentRule.SNAP_TO_TARGET,
                              unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.KEEP_WORLD, False)
    component = vfx_actor.niagara_component
    component.activate(True)
    out.append("system %s active %s" % (component.get_asset().get_name(), component.is_active()))

    editor.set_level_viewport_camera_info(
        unreal.Vector(LOCATION.x, LOCATION.y - 150.0, LOCATION.z + CAMERA_HEIGHT), unreal.Rotator(0.0, -90.0, 0.0))
    return out


try:
    build()
except Exception:
    out.append("FAILED\n" + traceback.format_exc())

open(REPORT, "w").write("\n".join(out))
