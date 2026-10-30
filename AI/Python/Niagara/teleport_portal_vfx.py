"""The teleport portal's Niagara system, NS_TeleportPortal: one emitter, intro and outro included.

One emitter (`Minimal`) on the life-phase modules (`AI/VFX.md`, "Life phases"): it plays INTRO, holds MAIN for as
long as the emitter is active and plays OUTRO once the system is deactivated (a graceful Deactivate, which lets the
particle finish on its own). The sprite wears M_TeleportPortal, centred and facing the camera, as wide as the
material's quad, and its `Color` bound to User.Color. The phases' size module is disabled and a stock
ScaleSpriteSize scales the sprite uniformly by the brightness of the phase-faded particle colour, which is also the
fade the material reads.

The script retunes the system in place, on the emitter the asset already holds: `prepare()` resets the renderer,
`main()` rewrites every value and the binding, and adds the scale module only while `ADD_SCALE_MODULE` is set (a
rerun on a system that has it would stack a second one).

The pad calls Activate and Deactivate (AGeoTeleporter::ApplyOpen) and nothing else.

Run through MCP execute_script (`prepare()`, then `main()`); the report goes to AI/Output/teleport_portal_vfx.txt.
Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import traceback

import unreal

SYSTEM_NAME = "NS_TeleportPortal"
EMITTER = "Minimal"
TARGET_FOLDER = "/Game/Art/VFX/Teleporter"
PORTAL_MATERIAL = "/Game/Art/VFX/Teleporter/M_TeleportPortal.M_TeleportPortal"

PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT
INITIALIZE_PARTICLE = "InitializeParticle"
PHASE_SIZE = "NM_LifePhases_SpriteSize"
SCALE_SPRITE_SIZE = "ScaleSpriteSize"
MODULE_SCALE_SPRITE_SIZE = "/Niagara/Modules/Update/Size/ScaleSpriteSize.ScaleSpriteSize"
DYN_FLOAT_FROM_COLOR = "/Niagara/DynamicInputs/TypeConversions/MakeFloatFromLinearColor.MakeFloatFromLinearColor"

QUAD_SIZE = 464.0  # the material's quad: the artifact's 200-unit disc, 16 % wider for the sparks
HELD = 100000.0  # the beam's lifetime parameter: long enough that only Deactivate ends the portal
ADD_SCALE_MODULE = False  # True only on a system whose stack has no ScaleSpriteSize yet
PORTAL_BLUE = "0.0685,0.3915,1,1"
INTRO_SECONDS = 0.5
OUTRO_SECONDS = 0.9
PHASE_STATE = "NM_LifePhases_ParticleState"

LOG = []


def builder():
    return unreal.GeoNiagaraBuilderUtil.get_default_object()


def record(label, value):
    LOG.append("%-58s %s" % (label, value))
    return value


def system_path():
    return "%s/%s.%s" % (TARGET_FOLDER, SYSTEM_NAME, SYSTEM_NAME)


def prepare():
    """Rewrites the renderer of the system's emitter."""
    system = unreal.load_object(None, system_path())
    record(SYSTEM_NAME, system.get_path_name() if system else "MISSING")

    sprite = unreal.find_object(unreal.find_object(system, EMITTER), "NiagaraSpriteRendererProperties_0")
    sprite.set_editor_property("Material", unreal.load_object(None, PORTAL_MATERIAL))
    sprite.set_editor_property("PivotInUVSpace", unreal.Vector2D(0.5, 0.5))
    sprite.set_editor_property("Alignment", unreal.NiagaraSpriteAlignment.UNALIGNED)
    sprite.set_editor_property("FacingMode", unreal.NiagaraSpriteFacingMode.FACE_CAMERA)
    record("renderer", "material, pivot, alignment, facing")
    unreal.EditorAssetLibrary.save_loaded_asset(system)


def write(system, usage, node, input_name, value):
    record("%s %s" % (node, input_name),
           builder().set_input_value(system, EMITTER, usage, node, input_name, str(value)))


def scale_by_fade(system):
    """Uniform scale = the brightest channel of the particle colour: 0 -> 1 in, 1 -> 0 out, with the phases."""
    cdo = builder()
    record("disable phase size", cdo.set_module_enabled(system, EMITTER, PARTICLE_UPDATE, PHASE_SIZE, False))
    record("scale module", cdo.add_module(system, EMITTER, PARTICLE_UPDATE, MODULE_SCALE_SPRITE_SIZE))
    record("scale mode", cdo.set_static_switch(system, EMITTER, PARTICLE_UPDATE, SCALE_SPRITE_SIZE,
                                               "Scale Sprite Size Mode", "Uniform"))
    record("compile scale module", cdo.compile_and_save(system))
    reader = record("scale factor", cdo.set_input_dynamic_input(
        system, EMITTER, PARTICLE_UPDATE, SCALE_SPRITE_SIZE, "Uniform Scale Factor", DYN_FLOAT_FROM_COLOR))
    record("compile scale factor", cdo.compile_and_save(system))
    record("factor color", cdo.set_input_linked_parameter(
        system, EMITTER, PARTICLE_UPDATE, str(reader), "LinearColor", "Particles.Color"))
    record("factor channel", cdo.set_static_switch(system, EMITTER, PARTICLE_UPDATE, str(reader), "Channel", "Value"))


def main():
    cdo = builder()
    system = system_path()
    for name, value in (("User.Beam_Length", QUAD_SIZE), ("User.Beam_Width", QUAD_SIZE),
                        ("User.LifeTime", HELD), ("User.Color", PORTAL_BLUE)):
        record(name, cdo.set_user_parameter(system, name, str(value)))
    write(system, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Position Offset", "0,0,0")
    write(system, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", QUAD_SIZE)
    write(system, PARTICLE_UPDATE, PHASE_STATE, "INTRO_Lifetime", INTRO_SECONDS)
    write(system, PARTICLE_UPDATE, PHASE_STATE, "OUTRO_Lifetime", OUTRO_SECONDS)
    if ADD_SCALE_MODULE:
        scale_by_fade(system)
    record("bind Color", cdo.bind_material_parameter_to_user_parameter(system, EMITTER, "Color", "User.Color"))
    record("compile", cdo.compile_and_save(system))


if __name__ == "__main__":
    report = unreal.Paths.project_dir() + "AI/Output/teleport_portal_vfx.txt"
    try:
        prepare()
        main()
    except Exception:
        record("ERROR", traceback.format_exc())
    open(report, "w").write("\n".join(LOG))
