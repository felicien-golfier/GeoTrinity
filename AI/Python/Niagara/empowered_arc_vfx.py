"""An electric arc that runs over a character's own mesh.

Builds /Game/Art/VFX/Generic/Niagara/NS_EmpoweredArcRun.

Every layer takes its spawn point from SkeletalMeshLocation sampling the character's own skeletal mesh, so one
asset fits Square, Circle and Triangle and needs no per-class variant. The module's data interface is left on
its default source mode, which falls back to the component the system is attached to when nothing sets a
source, so attaching the system to the character is the whole hookup - see ENDISkeletalMesh_SourceMode::Default
in NiagaraDataInterfaceSkeletalMesh.h. Surface sampling covers the whole shape rather than only its rim.

Where a strike lands is decided per particle rather than per emitter. A random dynamic input carries an
evaluation switch, and evaluating at spawn resolves it once for whatever spawned - once per particle in a
particle script, but once for the emitter's entire life in an emitter script. A bolt whose endpoints are
emitter scope therefore keeps the endpoints it drew on its first frame, however often it restrikes. Seeding
from a particle module instead gives every particle its own draw, so the arc lands somewhere new each time it
restrikes and never settles into a fixed place.

The three layers differ by mechanism:

  layer      renderer  placement          motion                     emission
  Run        sprite    mesh surface       linear drift vs drag       continuous rate
  Emanation  sprite    mesh surface       outward push vs drag       continuous rate
  Strike     sprite    mesh surface       none, it only flashes      restriking bursts

Run is what reads as electricity travelling: its sprites are velocity aligned and sized long on one axis and
thin on the other, so each particle draws a streak lying along its own direction of travel rather than a dot.
Jitter breaks each streak off a straight path, and drag settles the drift so the streaks stay near the body.

Strike never falls into a beat: its burst carries a spawn probability, so it stays dark for whole loops at
random rather than flashing on a timer.

User.Radius and User.Lifetime are inherited from the base system and drive nothing here - the mesh decides
placement and scale, not a radius.

A run is `prepare()`, then the `niagara_ops` route to drop the base system's own emitter - the one step no
Python here reaches - then `main()`.

Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
TARGET_NAME = "NS_EmpoweredArcRun"
SYSTEM = "%s/%s.%s" % (TARGET_FOLDER, TARGET_NAME, TARGET_NAME)
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_CircleArround.NS_CircleArround"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_LayerSources"

TEMPLATE_SPRITE = "/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"
GLOW_MATERIAL = "/Game/Art/VFX/Generic/Materials/MatInstances/MI_Glow01.MI_Glow01"

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"

MODULE_SPAWN_RATE = "/Niagara/Modules/Emitter/SpawnRate.SpawnRate"
MODULE_SKELETAL_MESH_LOCATION = "/Niagara/Modules/Spawn/Location/V2/SkeletalMeshLocation.SkeletalMeshLocation"
MODULE_ADD_VELOCITY = "/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity"
MODULE_VELOCITY_FROM_POINT = "/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint.AddVelocityFromPoint"
MODULE_DRAG = "/Niagara/Modules/Update/Forces/Drag.Drag"
MODULE_JITTER_POSITION = "/Niagara/Modules/Update/Position/JitterPosition.JitterPosition"

DYN_RANDOM_VECTOR = "/Niagara/DynamicInputs/Random/V2/RandomVector.RandomVector"
DYN_MULTIPLY_VECTOR_BY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat"

MESH_SWITCHES = (("Mesh Sampling Type", "Surface (Triangles)"),
                 ("Position Sampling", "Apply (Rigid)"),
                 ("Triangle Sampling Mode", "Random (All Triangles)"),
                 ("Random Evaluation", "Spawn Only"))

RUN = "Run"
EMANATION = "Emanation"
STRIKE = "Strike"
LAYERS = (RUN, EMANATION, STRIKE)

RUN_RATE = 90.0
RUN_LIFETIME = 0.22
RUN_SIZE = "26,2.2"  # long on the travel axis, thin across it: a streak, not a dot
RUN_COLOR = "3.00,6.50,14.00,1"
RUN_SPEED = 240.0
RUN_DRAG = 3.0
RUN_JITTER = 3.5
RUN_JITTER_DELAY = 0.03

EMANATION_RATE = 45.0
EMANATION_LIFETIME = 0.45
EMANATION_SIZE = 4.0
EMANATION_COLOR = "5.00,8.00,14.00,1"
EMANATION_PUSH = 120.0
EMANATION_DRAG = 7.0

STRIKE_LOOP = 0.28
STRIKE_ODDS = 0.65
STRIKE_COUNT = 2
STRIKE_LIFETIME = 0.10
STRIKE_SIZE = 26.0
STRIKE_COLOR = "8.00,11.00,16.00,1"

NODES = {}
LOG = []


def builder():
    return unreal.GeoNiagaraBuilderUtil.get_default_object()


def record(label, value):
    LOG.append("%-58s %s" % (label, value))
    return value


def clear_asset(package_path):
    """Frees a package path; one the editor still holds is renamed aside instead."""
    if not unreal.EditorAssetLibrary.does_asset_exist(package_path):
        return True
    return (unreal.EditorAssetLibrary.delete_asset(package_path)
            or unreal.EditorAssetLibrary.rename_asset(package_path, package_path + "_Superseded"))


def duplicate_into(source_path, folder, name):
    clear_asset("%s/%s" % (folder, name))
    duplicate = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        name, folder, unreal.load_object(None, source_path))
    record(name, duplicate.get_path_name() if duplicate else "FAILED")
    if duplicate:
        unreal.EditorAssetLibrary.save_asset(duplicate.get_path_name())
    return duplicate


def prepare():
    """Rewrites the target and the per-layer source assets; the base system's own emitter goes next."""
    duplicate_into(BASE_SYSTEM, TARGET_FOLDER, TARGET_NAME)
    for layer in LAYERS:
        duplicate_into(TEMPLATE_SPRITE, LAYER_SOURCE_FOLDER, layer)
    return "\n".join(LOG)


def add_layer_emitters():
    for layer in LAYERS:
        record("emitter %s" % layer,
               builder().add_emitter(SYSTEM, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, layer, layer)))


def add_mesh_seed(layer):
    """The one module every layer shares: a fresh point on the character's own mesh, drawn per particle."""
    cdo = builder()
    node = str(cdo.add_module(SYSTEM, layer, PARTICLE_SPAWN, MODULE_SKELETAL_MESH_LOCATION))
    for switch_name, entry in MESH_SWITCHES:
        record("%s %s" % (layer, switch_name),
               cdo.set_static_switch(SYSTEM, layer, PARTICLE_SPAWN, node, switch_name, entry))
    record("%s lifetime mode" % layer, cdo.set_static_switch(
        SYSTEM, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime Mode", "Direct Set"))
    return node


def add_modules():
    cdo = builder()
    for layer in LAYERS:
        NODES[layer] = {"mesh": add_mesh_seed(layer)}

    NODES[RUN]["rate"] = str(cdo.add_module(SYSTEM, RUN, EMITTER_UPDATE, MODULE_SPAWN_RATE))
    NODES[RUN]["velocity"] = str(cdo.add_module(SYSTEM, RUN, PARTICLE_SPAWN, MODULE_ADD_VELOCITY))
    NODES[RUN]["drag"] = str(cdo.add_module(SYSTEM, RUN, PARTICLE_UPDATE, MODULE_DRAG, 0))
    NODES[RUN]["jitter"] = str(cdo.add_module(SYSTEM, RUN, PARTICLE_UPDATE, MODULE_JITTER_POSITION))
    record("%s sprite size mode" % RUN, cdo.set_static_switch(
        SYSTEM, RUN, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Sprite Size Mode", "Non-Uniform"))

    NODES[EMANATION]["rate"] = str(cdo.add_module(SYSTEM, EMANATION, EMITTER_UPDATE, MODULE_SPAWN_RATE))
    NODES[EMANATION]["push"] = str(cdo.add_module(SYSTEM, EMANATION, PARTICLE_SPAWN,
                                                  MODULE_VELOCITY_FROM_POINT))
    NODES[EMANATION]["drag"] = str(cdo.add_module(SYSTEM, EMANATION, PARTICLE_UPDATE, MODULE_DRAG, 0))

    for layer, nodes in NODES.items():
        record("%s modules" % layer, nodes)


def attach_run_direction():
    """A random unit vector scaled by a vector whose Z is zero, so a streak stays in the camera's plane."""
    cdo = builder()
    NODES[RUN]["random"] = record("%s random direction" % RUN, str(cdo.set_input_dynamic_input(
        SYSTEM, RUN, PARTICLE_SPAWN, NODES[RUN]["velocity"], "Velocity", DYN_RANDOM_VECTOR)))


def attach_run_scale():
    cdo = builder()
    NODES[RUN]["scale"] = record("%s direction scale" % RUN, str(cdo.set_input_dynamic_input(
        SYSTEM, RUN, PARTICLE_SPAWN, NODES[RUN]["random"], "Vector Scale", DYN_MULTIPLY_VECTOR_BY_FLOAT)))


def write(layer, usage, node, input_name, value):
    record("%s %s %s" % (layer, node, input_name),
           builder().set_input_value(SYSTEM, layer, usage, node, input_name, str(value)))


def loop_forever(layer):
    record("%s loop behavior" % layer, builder().set_static_switch(
        SYSTEM, layer, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite"))


def emit_continuously(layer, rate):
    """A rate reads as a state; the template's burst would read as an event, so it is switched off."""
    record("%s disable burst" % layer,
           builder().set_module_enabled(SYSTEM, layer, EMITTER_UPDATE, BURST, False))
    write(layer, EMITTER_UPDATE, NODES[layer]["rate"], "SpawnRate", rate)


def set_run_values():
    loop_forever(RUN)
    emit_continuously(RUN, RUN_RATE)
    write(RUN, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", RUN_LIFETIME)
    write(RUN, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", RUN_COLOR)
    write(RUN, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Sprite Size", RUN_SIZE)
    write(RUN, PARTICLE_SPAWN, NODES[RUN]["scale"], "Vector", "%s,%s,0" % (RUN_SPEED, RUN_SPEED))
    write(RUN, PARTICLE_SPAWN, NODES[RUN]["scale"], "Float", 1)
    write(RUN, PARTICLE_UPDATE, NODES[RUN]["drag"], "Drag", RUN_DRAG)
    write(RUN, PARTICLE_UPDATE, NODES[RUN]["jitter"], "Jitter Amount", RUN_JITTER)
    write(RUN, PARTICLE_UPDATE, NODES[RUN]["jitter"], "Jitter Delay", RUN_JITTER_DELAY)


def set_emanation_values():
    loop_forever(EMANATION)
    emit_continuously(EMANATION, EMANATION_RATE)
    write(EMANATION, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", EMANATION_LIFETIME)
    write(EMANATION, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", EMANATION_COLOR)
    write(EMANATION, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", EMANATION_SIZE)
    write(EMANATION, PARTICLE_SPAWN, NODES[EMANATION]["push"], "Velocity Strength", EMANATION_PUSH)
    write(EMANATION, PARTICLE_UPDATE, NODES[EMANATION]["drag"], "Drag", EMANATION_DRAG)


def set_strike_values():
    loop_forever(STRIKE)
    write(STRIKE, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", STRIKE_LOOP)
    for input_name, value in (("Spawn Count", STRIKE_COUNT), ("Spawn Probability", STRIKE_ODDS),
                              ("Spawn Time", 0), ("Loop Count Limit", 0)):
        write(STRIKE, EMITTER_UPDATE, BURST, input_name, value)
    write(STRIKE, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", STRIKE_LIFETIME)
    write(STRIKE, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", STRIKE_COLOR)
    write(STRIKE, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", STRIKE_SIZE)


def renderer(layer):
    """Run's sprites align to velocity so each draws a streak; the others stay on the template's alignment."""
    emitter = unreal.find_object(unreal.load_object(None, SYSTEM), layer)
    properties = unreal.find_object(emitter, "NiagaraSpriteRendererProperties_0") if emitter else None
    if not properties:
        record("%s renderer" % layer, "NOT FOUND")
        return
    properties.set_editor_property("Material", unreal.load_object(None, GLOW_MATERIAL))
    if layer == RUN:
        properties.set_editor_property("Alignment", unreal.NiagaraSpriteAlignment.VELOCITY_ALIGNED)
        properties.set_editor_property("FacingMode", unreal.NiagaraSpriteFacingMode.FACE_CAMERA)
    record("%s renderer" % layer, "%s %s" % (properties.get_editor_property("Material").get_name(),
                                             properties.get_editor_property("Alignment")))


def main():
    """Each phase runs before the compile that makes the nodes it added addressable by value."""
    cdo = builder()
    add_layer_emitters()
    for phase in (add_modules, attach_run_direction, attach_run_scale):
        phase()
        record("compile after %s" % phase.__name__, cdo.compile_and_save(SYSTEM))
    for setter in (set_run_values, set_emanation_values, set_strike_values):
        setter()
    for layer in LAYERS:
        renderer(layer)
    record("compile", cdo.compile_and_save(SYSTEM))
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_EmpoweredArc.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(main())
