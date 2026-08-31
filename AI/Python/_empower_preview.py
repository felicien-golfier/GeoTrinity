"""Attaches NS_Empowerment to every playable character in the running PIE session.

Judge a mesh-sampled system on a character in a level, never in the asset editor preview: the preview has
no attach parent, so SkeletalMeshLocation resolves to nothing there and the mesh-fitting layers show
empty. The game camera is also the only place the top-down orthographic framing this was authored for
actually applies.
"""

import unreal
import traceback

SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_Empowerment.NS_Empowerment"
REPORT = "%sGeoTrinity_Preview.txt" % unreal.Paths.project_saved_dir()

out = []


def pie_world():
    """Constructing the subsystem gives a transient object with no world; only the getter carries one."""
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


try:
    world = pie_world()
    out.append("world %s" % (world.get_path_name() if world else "NONE"))
    system = unreal.load_object(None, SYSTEM)
    pawns = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Character)]
    out.append("pawns %s" % [(p.get_name(), p.get_class().get_name()) for p in pawns])
    for pawn in pawns:
        component = unreal.NiagaraFunctionLibrary.spawn_system_attached(
            system, pawn.get_editor_property("mesh"), "None",
            unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0),
            unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True)
        out.append("attached to %s -> %s" % (pawn.get_name(), component))
except Exception:
    out.append("FAILED\n" + traceback.format_exc())

open(REPORT, "w").write("\n".join(out))
