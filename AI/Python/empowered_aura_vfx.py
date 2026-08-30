"""Three empowerment auras: light flames wrapping a character.

Builds /Game/Art/VFX/Generic/Niagara/NS_EmpoweredBlaze, NS_EmpoweredSurge and NS_EmpoweredArcStorm.

The flames are sampled off the character's own skeletal mesh, so one asset fits Square, Circle and Triangle
with no per-class variant. SkeletalMeshLocation's data interface is left on its default source mode, which
falls back to the component the system is attached to when nothing sets a source, so attaching the system to
the character is the whole hookup - see ENDISkeletalMesh_SourceMode::Default in NiagaraDataInterfaceSkeletalMesh.h.
Surface sampling covers the whole shape rather than only its rim, which is what makes the body itself read as
burning instead of merely outlined.

The three differ by the mechanism that moves the fire, not by tuning:

  system               flame motion
  NS_EmpoweredBlaze    radial push vs drag, curl on top
  NS_EmpoweredSurge    vortex wound about the up axis
  NS_EmpoweredArcStorm no push at all, crackle in place

Layers emit at a continuous rate rather than in bursts - a rate reads as an ambient state, a burst as a series
of events, and empowerment is a state.

Electricity is not built here; it lives in AI/Python/empowered_arc_vfx.py.

A run is `prepare()`, then the `niagara_ops` route to drop each base system's own emitter - the one step no
Python here reaches - then `main()`.

Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
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
MODULE_VELOCITY_FROM_POINT = "/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint.AddVelocityFromPoint"
MODULE_CURL_NOISE_FORCE = "/Niagara/Modules/Update/Forces/CurlNoiseForce.CurlNoiseForce"
MODULE_DRAG = "/Niagara/Modules/Update/Forces/Drag.Drag"
MODULE_VORTEX_VELOCITY = "/Niagara/Modules/Update/Velocity/VortexVelocity.VortexVelocity"

AURA_RADIUS = 50.0  # matches the playable character's capsule radius
CURL_FREQUENCY = 0.03

# name, rate, lifetime, size, colour, push, drag, curl, vortex
SYSTEMS = {
    "NS_EmpoweredBlaze": (
        ("Flame", 150.0, 0.30, 16.0, "6.00,3.20,0.60,1", 170.0, 6.0, 260.0, 0.0),
        ("Ember", 40.0, 0.80, 6.0, "9.00,6.00,2.00,1", 70.0, 2.5, 140.0, 0.0),
    ),
    "NS_EmpoweredSurge": (
        ("Spiral", 190.0, 0.55, 13.0, "5.00,2.60,0.80,1", 45.0, 1.6, 90.0, 260.0),
        ("Crown", 55.0, 0.95, 7.0, "8.00,5.00,1.60,1", 25.0, 1.2, 60.0, 120.0),
    ),
    "NS_EmpoweredArcStorm": (
        ("Crackle", 220.0, 0.14, 8.0, "3.00,4.00,9.00,1", 18.0, 9.0, 320.0, 0.0),
    ),
}

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


def system_path(system_name):
    return "%s/%s.%s" % (TARGET_FOLDER, system_name, system_name)


def prepare():
    """Rewrites the three targets and every layer source; dropping each base's own emitter goes next."""
    for system_name, layers in SYSTEMS.items():
        duplicate_into(BASE_SYSTEM, TARGET_FOLDER, system_name)
        for layer in layers:
            duplicate_into(TEMPLATE_SPRITE, LAYER_SOURCE_FOLDER, layer[0])
    return "\n".join(LOG)


def add_layer_emitters(system_name, layers):
    for layer in layers:
        record("%s emitter %s" % (system_name, layer[0]), builder().add_emitter(
            system_path(system_name), "%s/%s.%s" % (LAYER_SOURCE_FOLDER, layer[0], layer[0])))


def add_modules(system, layer):
    """Forces go in above the solver that reads them, so each is inserted rather than appended."""
    cdo = builder()
    name = layer[0]
    NODES[name] = {
        "rate": str(cdo.add_module(system, name, EMITTER_UPDATE, MODULE_SPAWN_RATE)),
        "mesh": str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_SKELETAL_MESH_LOCATION)),
        "push": str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_VELOCITY_FROM_POINT)),
        "curl": str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_CURL_NOISE_FORCE, 0)),
        "drag": str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_DRAG, 1)),
    }
    if layer[8]:
        NODES[name]["vortex"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_VORTEX_VELOCITY, 2))
    record("%s modules" % name, NODES[name])


def set_values(system, layer):
    name, rate, lifetime, size, color, push, drag, curl, vortex = layer
    cdo = builder()
    nodes = NODES[name]

    def write(usage, node, input_name, value):
        record("%s %s %s" % (name, node, input_name),
               cdo.set_input_value(system, name, usage, node, input_name, str(value)))

    record("%s loop behavior" % name, cdo.set_static_switch(
        system, name, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite"))
    record("%s disable burst" % name,
           cdo.set_module_enabled(system, name, EMITTER_UPDATE, BURST, False))
    write(EMITTER_UPDATE, nodes["rate"], "SpawnRate", rate)

    for switch_name, entry in (("Mesh Sampling Type", "Surface (Triangles)"),
                               ("Position Sampling", "Apply (Rigid)"),
                               ("Triangle Sampling Mode", "Random (All Triangles)")):
        record("%s %s" % (name, switch_name), cdo.set_static_switch(
            system, name, PARTICLE_SPAWN, nodes["mesh"], switch_name, entry))
    record("%s lifetime mode" % name, cdo.set_static_switch(
        system, name, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime Mode", "Direct Set"))

    write(PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", lifetime)
    write(PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", color)
    write(PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", size)
    write(PARTICLE_SPAWN, nodes["push"], "Velocity Strength", push)

    write(PARTICLE_UPDATE, nodes["drag"], "Drag", drag)
    write(PARTICLE_UPDATE, nodes["curl"], "Noise Strength", curl)
    write(PARTICLE_UPDATE, nodes["curl"], "Noise Frequency", CURL_FREQUENCY)
    if vortex:
        write(PARTICLE_UPDATE, nodes["vortex"], "Velocity Amount", vortex)
        write(PARTICLE_UPDATE, nodes["vortex"], "Vortex Axis", "0,0,1")
        write(PARTICLE_UPDATE, nodes["vortex"], "Influence Falloff Radius", AURA_RADIUS * 3.0)


def renderer(system_name, layer):
    name = layer[0]
    emitter = unreal.find_object(unreal.load_object(None, system_path(system_name)), name)
    properties = unreal.find_object(emitter, "NiagaraSpriteRendererProperties_0") if emitter else None
    if not properties:
        record("%s renderer" % name, "NOT FOUND")
        return
    properties.set_editor_property("Material", unreal.load_object(None, GLOW_MATERIAL))
    record("%s renderer" % name, properties.get_editor_property("Material").get_name())


def build(system_name, layers):
    """Modules are added across every layer before the compile that makes their values addressable."""
    cdo = builder()
    system = system_path(system_name)
    add_layer_emitters(system_name, layers)
    record("%s User.Radius" % system_name, cdo.set_user_parameter(system, "User.Radius", str(AURA_RADIUS)))

    for layer in layers:
        add_modules(system, layer)
    record("%s compile modules" % system_name, cdo.compile_and_save(system))

    for layer in layers:
        set_values(system, layer)
        renderer(system_name, layer)
    record("%s compile" % system_name, cdo.compile_and_save(system))


def main():
    for system_name, layers in SYSTEMS.items():
        build(system_name, layers)
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_EmpoweredAura.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(main())
