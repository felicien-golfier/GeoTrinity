"""Three more electric declinations, each built on a mechanism the other systems do not use.

Builds /Game/Art/VFX/Generic/Niagara/NS_SparkFizz, NS_OrbitArc and NS_ShardStorm.

Together with NS_StaticElectricity (bolts struck between random rim points) and NS_ConeCoil (a helix wound on
a cone and turned about its axis), these cover five ways of reading as electricity, and no two share a
mechanism or a renderer:

  system              renderer   placement              motion                emission
  NS_StaticElectricity ribbon    random beam endpoints  curl noise + drag     restriking bursts
  NS_ConeCoil          ribbon    exec index helix       vortex velocity       one long strand
  NS_SparkFizz         sprite    random ring surface    outward push vs drag  continuous rate
  NS_OrbitArc          ribbon    orbiting beam endpoints re-derived per frame  one held strand
  NS_ShardStorm        mesh      random ring surface    inward attraction     restriking bursts

Each system carries `User.Radius`, so all five fit the same character from one number.

NS_SparkFizz is the only one that never draws a line: sprites thrown off the rim and dragged to a halt within
a few centimetres, so what the eye follows is a fizz of dying embers rather than a strand. It is also the only
one emitting at a continuous rate instead of in bursts, which is what makes it read as an ambient state rather
than as a series of events.

NS_OrbitArc is the one strand that never stops moving. Its two beam endpoints are a cosine and a sine of the
emitter's age, so they circle the character at unrelated rates, and UpdateBeam re-derives every particle's
position from them each frame — the same burst of particles is redrawn along a chord that sweeps and stretches
continuously. That module is what separates a beam that follows its endpoints from one frozen at spawn, and it
has to sit above the jitter that shakes the strand, or the re-derivation overwrites the shake.

NS_ShardStorm answers the same brief with no strand at all: mesh shards scattered on the rim, spinning on
their own axes and pulled inward by a point attraction, so the read is debris caught in a field rather than a
discharge. Its template's upward velocity is disabled — an orthographic camera looking down sees nothing of it.

A run is `prepare()`, then the `niagara_ops` route to drop each base system's own emitter — the one step no
Python here reaches — then `main()`.

Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_CircleArround.NS_CircleArround"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_LayerSources"
RIBBON_MATERIAL = "/Niagara/DefaultAssets/DefaultRIbbonMaterial.DefaultRibbonMaterial"

TEMPLATE_SPRITE = "/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"
TEMPLATE_BEAM = "/Niagara/DefaultAssets/Templates/Emitters/StaticBeam.StaticBeam"
TEMPLATE_MESH = "/Niagara/DefaultAssets/Templates/Emitters/UpwardMeshBurst.UpwardMeshBurst"

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"
BEAM_SETUP = "BeamEmitterSetup"
UPWARD_VELOCITY = "AddVelocity"

MODULE_SHAPE_LOCATION = "/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"
MODULE_SPAWN_RATE = "/Niagara/Modules/Emitter/SpawnRate.SpawnRate"
MODULE_VELOCITY_FROM_POINT = "/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint.AddVelocityFromPoint"
MODULE_DRAG = "/Niagara/Modules/Update/Forces/Drag.Drag"
MODULE_SCALE_COLOR = "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"
MODULE_BEAM_SETUP = "/Niagara/Modules/Beams/BeamEmitterSetup.BeamEmitterSetup"
MODULE_UPDATE_BEAM = "/Niagara/Modules/Beams/UpdateBeam.UpdateBeam"
MODULE_JITTER_POSITION = "/Niagara/Modules/Update/Position/JitterPosition.JitterPosition"
MODULE_MESH_ROTATION_RATE = "/Niagara/Modules/Update/Orientation/MeshRotationRate.MeshRotationRate"
MODULE_POINT_ATTRACTION = "/Niagara/Modules/Update/Forces/PointAttractionForce.PointAttractionForce"

DYN_ADD_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Vectors/Position/AddVectorToPosition.AddVectorToPosition"
DYN_CONVERT_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Transforms/ConvertVectorToPosition.ConvertVectorToPosition"
DYN_SIMULATION_POSITION = "/Niagara/DynamicInputs/Helpers/SimulationPosition.SimulationPosition"
DYN_MAKE_VECTOR = "/Niagara/DynamicInputs/TypeConversions/MakeVector.MakeVector"
DYN_COSINE = "/Niagara/DynamicInputs/Angles/Cosine.Cosine"
DYN_SINE = "/Niagara/DynamicInputs/Angles/Sine.Sine"
DYN_MULTIPLY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_Float.Multiply_Float"
DYN_ONE_MINUS_FLOAT = "/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat"

RADIUS = 50.0  # matches the playable character's capsule radius
CURVE_TENSION = 0.99  # tension IS sharpness: a slack ribbon rounds its corners off and reads as a noodle

LOG = []


def builder():
    return unreal.GeoNiagaraBuilderUtil.get_default_object()


def record(label, value):
    LOG.append("%-58s %s" % (label, value))
    return value


def clear_asset(package_path):
    """Frees a package path so an asset can be written to it; one still held open is renamed aside instead."""
    if not unreal.EditorAssetLibrary.does_asset_exist(package_path):
        return True
    return (unreal.EditorAssetLibrary.delete_asset(package_path)
            or unreal.EditorAssetLibrary.rename_asset(package_path, package_path + "_Superseded"))


def duplicate_into(source_path, folder, name):
    """Duplicates over folder/name; duplication returns nothing when the name is already taken."""
    clear_asset("%s/%s" % (folder, name))
    duplicate = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        name, folder, unreal.load_object(None, source_path))
    record(name, duplicate.get_path_name() if duplicate else "FAILED")
    if duplicate:
        unreal.EditorAssetLibrary.save_asset(duplicate.get_path_name())
    return duplicate


def system_path(name):
    return "%s/%s.%s" % (TARGET_FOLDER, name, name)


def write(system, emitter, usage, node, input_name, value):
    record("%s %s %s" % (emitter, node, input_name),
           builder().set_input_value(system, emitter, usage, node, input_name, str(value)))


def switch(system, emitter, usage, node, switch_name, entry):
    record("%s %s %s" % (emitter, node, switch_name),
           builder().set_static_switch(system, emitter, usage, node, switch_name, entry))


def nest(system, emitter, usage, node, input_name, script):
    return record("%s %s %s" % (emitter, node, input_name), str(
        builder().set_input_dynamic_input(system, emitter, usage, node, input_name, script)))


def emitter_cadence(system, emitter, loop, count, odds):
    """Every variant loops forever; the burst count is the whole population of one strike."""
    switch(system, emitter, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    write(system, emitter, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", loop)
    for input_name, value in (("Spawn Count", count), ("Spawn Probability", odds), ("Spawn Time", 0),
                              ("Loop Count Limit", 0)):
        write(system, emitter, EMITTER_UPDATE, BURST, input_name, value)


def ring_placement(system, emitter, node, factor):
    """A random point on the rim; the ring lies in XY, which is the plane the camera reads."""
    for switch_name, entry in (("Shape Primitive", "Ring / Disc"), ("Ring / Disc Mode", "Circle")):
        switch(system, emitter, PARTICLE_SPAWN, node, switch_name, entry)
    builder().compile_and_save(system)
    radius = nest(system, emitter, PARTICLE_SPAWN, node, "Ring Radius", DYN_MULTIPLY_FLOAT)
    builder().compile_and_save(system)
    record("%s ring radius" % emitter, builder().set_input_linked_parameter(
        system, emitter, PARTICLE_SPAWN, radius, "A", "User.Radius"))
    write(system, emitter, PARTICLE_SPAWN, radius, "B", factor)


def fade_over_life(system, emitter, node):
    alpha = nest(system, emitter, PARTICLE_UPDATE, node, "Scale Alpha", DYN_ONE_MINUS_FLOAT)
    builder().compile_and_save(system)
    record("%s alpha from age" % emitter, builder().set_input_linked_parameter(
        system, emitter, PARTICLE_UPDATE, alpha, "Float", "Particles.NormalizedAge"))


def renderer(system, emitter, subobject, ribbon=False, shard_scale=0.0):
    """A mesh renderer carries its own scale, which is the only reach a stack edit has on shard size."""
    properties = unreal.find_object(unreal.find_object(unreal.load_object(None, system), emitter), subobject)
    if not properties:
        record("%s renderer" % emitter, "NOT FOUND")
        return
    if ribbon:
        properties.set_editor_property("Material", unreal.load_object(None, RIBBON_MATERIAL))
        properties.set_editor_property("CurveTension", CURVE_TENSION)
    if shard_scale:
        meshes = properties.get_editor_property("Meshes")
        for mesh in meshes:
            mesh.set_editor_property("Scale", unreal.Vector(shard_scale, shard_scale, shard_scale))
        properties.set_editor_property("Meshes", meshes)
        record("%s shard scale" % emitter, len(meshes))
    record("%s renderer" % emitter, subobject)


def build_spark_fizz(system, emitter):
    """Embers off the rim: a continuous rate, an outward push, and a size that falls away with the speed."""
    cdo = builder()
    rate = str(cdo.add_module(system, emitter, EMITTER_UPDATE, MODULE_SPAWN_RATE))
    record("%s burst off" % emitter, cdo.set_module_enabled(system, emitter, EMITTER_UPDATE, BURST, False))
    shape = str(cdo.add_module(system, emitter, PARTICLE_SPAWN, MODULE_SHAPE_LOCATION))
    push = str(cdo.add_module(system, emitter, PARTICLE_SPAWN, MODULE_VELOCITY_FROM_POINT))
    drag = str(cdo.add_module(system, emitter, PARTICLE_UPDATE, MODULE_DRAG, 0))
    fade = str(cdo.add_module(system, emitter, PARTICLE_UPDATE, MODULE_SCALE_COLOR))
    record("%s modules" % emitter, (rate, shape, push, drag, fade))
    cdo.compile_and_save(system)

    switch(system, emitter, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    write(system, emitter, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", 1.0)
    write(system, emitter, EMITTER_UPDATE, rate, "SpawnRate", 90)
    ring_placement(system, emitter, shape, 1.0)
    for switch_name, entry in (("Lifetime Mode", "Direct Set"), ("Color Mode", "Direct Set")):
        switch(system, emitter, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch_name, entry)
    cdo.compile_and_save(system)
    write(system, emitter, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", 0.28)
    write(system, emitter, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", "0.70,1.60,3.60,1")
    write(system, emitter, PARTICLE_SPAWN, push, "Velocity Strength", 150)
    write(system, emitter, PARTICLE_UPDATE, drag, "Drag", 9.0)
    fade_over_life(system, emitter, fade)
    renderer(system, emitter, "NiagaraSpriteRendererProperties_0")


def build_orbit_arc(system, emitter):
    """One strand held between two circling endpoints, redrawn from them every frame."""
    cdo = builder()
    record("%s template beam off" % emitter,
           cdo.set_module_enabled(system, emitter, EMITTER_UPDATE, BEAM_SETUP, False))
    beam = str(cdo.add_module(system, emitter, EMITTER_UPDATE, MODULE_BEAM_SETUP))
    follow = str(cdo.add_module(system, emitter, PARTICLE_UPDATE, MODULE_UPDATE_BEAM, 0))
    shake = str(cdo.add_module(system, emitter, PARTICLE_UPDATE, MODULE_JITTER_POSITION))
    record("%s modules" % emitter, (beam, follow, shake))
    cdo.compile_and_save(system)

    # The start is an absolute position and the end an offset the module adds the emitter's own position to.
    start = nest(system, emitter, EMITTER_UPDATE, beam, "Beam Start", DYN_ADD_VECTOR_TO_POSITION)
    end = nest(system, emitter, EMITTER_UPDATE, beam, "Beam End", DYN_CONVERT_VECTOR_TO_POSITION)
    cdo.compile_and_save(system)
    record("%s anchor" % emitter, cdo.set_input_dynamic_input(
        system, emitter, EMITTER_UPDATE, start, "Position", DYN_SIMULATION_POSITION))
    orbits = (nest(system, emitter, EMITTER_UPDATE, start, "Vector", DYN_MAKE_VECTOR),
              nest(system, emitter, EMITTER_UPDATE, end, "Input Position", DYN_MAKE_VECTOR))
    cdo.compile_and_save(system)

    # A cosine on X against a sine on Y is a circle, and the trig feeds its own angle from time, so the period
    # is the only rate control there is. Unrelated periods on the two endpoints keep them from ever lining up,
    # so the chord between them never settles into a fixed length or a fixed angle.
    for orbit, period in zip(orbits, (3.1, -4.7)):
        angles = (nest(system, emitter, EMITTER_UPDATE, orbit, "X", DYN_COSINE),
                  nest(system, emitter, EMITTER_UPDATE, orbit, "Y", DYN_SINE))
        write(system, emitter, EMITTER_UPDATE, orbit, "Z", 0)
        cdo.compile_and_save(system)
        for angle in angles:
            write(system, emitter, EMITTER_UPDATE, angle, "Period", period)
            record("%s orbit radius" % emitter, cdo.set_input_linked_parameter(
                system, emitter, EMITTER_UPDATE, angle, "Scale", "User.Radius"))

    emitter_cadence(system, emitter, 2.0, 26, 1.0)
    switch(system, emitter, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime Mode", "Direct Set")
    cdo.compile_and_save(system)
    write(system, emitter, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", 1.8)
    write(system, emitter, PARTICLE_SPAWN, "FloatFromCurve002", "Scale Curve", 2.2)
    write(system, emitter, PARTICLE_UPDATE, "Color", "Color", "0.35,1.20,3.50,1")
    write(system, emitter, PARTICLE_UPDATE, shake, "Jitter Amount", 2.5)
    write(system, emitter, PARTICLE_UPDATE, shake, "Jitter Delay", 0.05)
    renderer(system, emitter, "NiagaraRibbonRendererProperties_0", True)


def build_shard_storm(system, emitter):
    """Debris in a field: shards on the rim, spinning, pulled toward the centre instead of thrown from it."""
    cdo = builder()
    record("%s upward off" % emitter,
           cdo.set_module_enabled(system, emitter, PARTICLE_SPAWN, UPWARD_VELOCITY, False))
    shape = str(cdo.add_module(system, emitter, PARTICLE_SPAWN, MODULE_SHAPE_LOCATION))
    spin = str(cdo.add_module(system, emitter, PARTICLE_UPDATE, MODULE_MESH_ROTATION_RATE))
    pull = str(cdo.add_module(system, emitter, PARTICLE_UPDATE, MODULE_POINT_ATTRACTION, 0))
    fade = str(cdo.add_module(system, emitter, PARTICLE_UPDATE, MODULE_SCALE_COLOR))
    record("%s modules" % emitter, (shape, spin, pull, fade))
    cdo.compile_and_save(system)

    emitter_cadence(system, emitter, 0.75, 16, 0.9)
    ring_placement(system, emitter, shape, 1.35)
    for switch_name, entry in (("Lifetime Mode", "Direct Set"), ("Color Mode", "Direct Set")):
        switch(system, emitter, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch_name, entry)
    cdo.compile_and_save(system)
    write(system, emitter, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", 0.65)
    write(system, emitter, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", "0.30,0.95,3.20,1")
    write(system, emitter, PARTICLE_UPDATE, spin, "Rotation Rate", 220)
    write(system, emitter, PARTICLE_UPDATE, pull, "AttractionStrength", 320)
    write(system, emitter, PARTICLE_UPDATE, pull, "Attraction Radius", 240)
    fade_over_life(system, emitter, fade)
    renderer(system, emitter, "NiagaraMeshRendererProperties_0", shard_scale=0.15)


VARIANTS = (
    ("NS_SparkFizz", "SparkFizz", TEMPLATE_SPRITE, build_spark_fizz),
    ("NS_OrbitArc", "OrbitArc", TEMPLATE_BEAM, build_orbit_arc),
    ("NS_ShardStorm", "ShardStorm", TEMPLATE_MESH, build_shard_storm),
)


def prepare():
    """Rewrites every target and its emitter source; each base system's own emitter goes next."""
    for name, emitter, template, _ in VARIANTS:
        duplicate_into(BASE_SYSTEM, TARGET_FOLDER, name)
        duplicate_into(template, LAYER_SOURCE_FOLDER, emitter)
    return "\n".join(LOG)


def main():
    cdo = builder()
    for name, emitter, _, build in VARIANTS:
        system = system_path(name)
        record("emitter %s" % emitter, cdo.add_emitter(system, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, emitter,
                                                                            emitter)))
        record("%s User.Radius" % name, cdo.set_user_parameter(system, "User.Radius", str(RADIUS)))
        cdo.compile_and_save(system)
        build(system, emitter)
        record("%s compile" % name, cdo.compile_and_save(system))
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_LightningVariants.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(main())
