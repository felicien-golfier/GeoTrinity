"""Static electricity aura: blue bolts striking around a character.

Builds the emitter stacks of /Game/Art/VFX/Generic/Niagara/NS_StaticElectricity.

A bolt is a beam. BeamEmitterSetup names two endpoints, SpawnBeam lays one burst of particles between them in
ribbon link order, and the ribbon renderer draws the strand. Each endpoint is a random unit vector flattened to
XY and scaled by `User.Radius`, taken around the emitter's own position, so a strike runs between two points of
the character's rim — XY is the plane the orthographic camera reads, and one number fits any character.

The template's own BeamEmitterSetup is disabled and a second one added rather than rewired: a dynamic input
can only be attached where the stack holds a plain value, and the template already feeds its beam start.

Those endpoints are re-rolled every frame, which is what makes them right: nothing re-derives a particle's
position after SpawnBeam, so a burst freezes the endpoints of the frame it fired on and the next burst picks
new ones. One burst per loop, with a lifetime shorter than the loop, keeps a single strand alive per emitter,
so the ribbon never chains two strikes into one snake.

What reads as lightning rather than as a glowing wire is that the strand is never straight and never still:

- `CurlNoiseLocation` offsets each particle at spawn, so a bolt is born jagged instead of snapping straight
  for its first frame.
- `CurlNoiseForce` against `Drag` bends the whole strand across its life. A curl field is spatially coherent,
  so neighbouring particles pull the same way and the strand writhes as a shape.
- `JitterPosition` re-rolls a small per-particle offset a few times per life — the crackle on top.

The split is the point: coarse motion coherent, fine motion random. Jitter alone, which was the whole of the
previous version, is incoherent at every scale and reads as buzzing noise rather than as a bolt.

Strikes never fall into a beat. Each layer loops on its own unrelated duration and each burst carries a spawn
probability, so a layer stays dark for whole loops at random and the three layers never resynchronise.

The base emitter is Niagara's StaticBeam template, which already carries the beam stack, a width curve indexed
by ribbon link order (the taper toward both ends) and a colour curve over particle age (the flash and decay).
Curve keys live in a data interface rather than in a rapid iteration constant and nothing here can write them,
so both curves are inherited and only their `Scale Curve` amplitude is set per layer.

A run is `prepare()`, then the `niagara_ops` route to drop the base system's own emitter — the one step no
Python here reaches — then `main()`, which adds the layer emitters back and builds their stacks. An added
emitter takes its handle name from its source asset, which is also the only way to name it.

Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
TARGET_NAME = "NS_StaticElectricity"
SYSTEM = "%s/%s.%s" % (TARGET_FOLDER, TARGET_NAME, TARGET_NAME)
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_CircleArround.NS_CircleArround"
LAYER_TEMPLATE = "/Niagara/DefaultAssets/Templates/Emitters/StaticBeam.StaticBeam"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_LayerSources"
RIBBON_MATERIAL = "/Niagara/DefaultAssets/DefaultRIbbonMaterial.DefaultRibbonMaterial"

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

BEAM_SETUP = "BeamEmitterSetup"
EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"
BEAM_WIDTH_CURVE = "FloatFromCurve002"
COLOR = "Color"

MODULE_BEAM_SETUP = "/Niagara/Modules/Beams/BeamEmitterSetup.BeamEmitterSetup"
MODULE_CURL_NOISE_LOCATION = "/Niagara/Modules/Spawn/Location/CurlNoiseLocation.CurlNoiseLocation"
MODULE_CURL_NOISE_FORCE = "/Niagara/Modules/Update/Forces/CurlNoiseForce.CurlNoiseForce"
MODULE_DRAG = "/Niagara/Modules/Update/Forces/Drag.Drag"
MODULE_JITTER_POSITION = "/Niagara/Modules/Update/Position/JitterPosition.JitterPosition"

DYN_ADD_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Vectors/Position/AddVectorToPosition.AddVectorToPosition"
DYN_CONVERT_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Transforms/ConvertVectorToPosition.ConvertVectorToPosition"
DYN_SIMULATION_POSITION = "/Niagara/DynamicInputs/Helpers/SimulationPosition.SimulationPosition"
DYN_RANDOM_VECTOR = "/Niagara/DynamicInputs/Random/V2/RandomVector.RandomVector"
DYN_MULTIPLY_VECTOR_BY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat"

AURA_RADIUS = 50.0  # matches the playable character's capsule radius

# The two noise fields do opposite jobs and the frequency is what separates them. A curl feature has to be
# SMALLER than the gap between two neighbouring points for the jag to zigzag — sampled coarser than that, the
# neighbours read almost the same value and the strand only shifts, staying as smooth as it was drawn. The
# force wants the opposite: a feature spanning the whole aura, so every point of a strand is pulled the same
# way and the bolt bends as one piece.
JAG_FREQUENCY = 0.25
CURL_FREQUENCY = 0.03
DRAG = 6.0  # against the curl force this settles a terminal speed, which is what bounds the bend
JITTER_DELAY = 0.04
CURVE_TENSION = 0.99  # tension IS sharpness: a slack ribbon rounds its corners off and reads as a noodle

# name, rim radius, points per bolt, loop seconds, strike odds, lifetime, width, colour,
# spawn jag, curl strength, jitter reach
LAYERS = (
    ("BoltRim", 1.00, 20, 0.17, 0.85, 0.14, 2.6, "0.55,1.30,3.40,1", 7.0, 320.0, 2.0),
    ("BoltCore", 0.55, 12, 0.11, 0.70, 0.08, 1.8, "1.40,2.00,3.20,1", 4.0, 220.0, 1.4),
    ("BoltHalo", 1.35, 24, 0.37, 0.50, 0.22, 3.6, "0.20,0.70,3.00,1", 10.0, 420.0, 2.6),
)

# input name, the node that feeds it, that node's own vector input, and whether it carries the emitter position.
# The two endpoints are not symmetric: the start is an absolute position, so the emitter's own has to be added
# under it, while the end is an offset the module adds that same position to. Handing the end a position too
# drops the emitter out of it and anchors that half of every strand at the world origin.
ENDPOINTS = (
    ("Beam Start", DYN_ADD_VECTOR_TO_POSITION, "Vector", True),
    ("Beam End", DYN_CONVERT_VECTOR_TO_POSITION, "Input Position", False),
)

NODES = {}
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


def prepare():
    """Rewrites the target and the per-layer source assets; the base system's own emitter goes next."""
    duplicate_into(BASE_SYSTEM, TARGET_FOLDER, TARGET_NAME)
    for layer in LAYERS:
        duplicate_into(LAYER_TEMPLATE, LAYER_SOURCE_FOLDER, layer[0])
    return "\n".join(LOG)


def add_layer_emitters():
    """Copies each source asset in as an emitter; the handle takes the source asset's name."""
    for layer in LAYERS:
        source = "%s/%s.%s" % (LAYER_SOURCE_FOLDER, layer[0], layer[0])
        record("emitter %s" % layer[0], builder().add_emitter(SYSTEM, source))


def add_modules(name):
    """The beam the endpoints go on, the jag at spawn, then the two forces — above the solver that reads them."""
    cdo = builder()
    record("%s disable template beam" % name,
           cdo.set_module_enabled(SYSTEM, name, EMITTER_UPDATE, BEAM_SETUP, False))
    NODES[name] = {
        "beam": str(cdo.add_module(SYSTEM, name, EMITTER_UPDATE, MODULE_BEAM_SETUP)),
        "jag": str(cdo.add_module(SYSTEM, name, PARTICLE_SPAWN, MODULE_CURL_NOISE_LOCATION)),
        "curl": str(cdo.add_module(SYSTEM, name, PARTICLE_UPDATE, MODULE_CURL_NOISE_FORCE, 0)),
        "drag": str(cdo.add_module(SYSTEM, name, PARTICLE_UPDATE, MODULE_DRAG, 1)),
        "jitter": str(cdo.add_module(SYSTEM, name, PARTICLE_UPDATE, MODULE_JITTER_POSITION)),
    }
    record("%s modules" % name, NODES[name])


def attach_endpoints(name):
    cdo = builder()
    for endpoint, feeder, _, _ in ENDPOINTS:
        NODES[name][endpoint] = record("%s %s" % (name, endpoint), str(cdo.set_input_dynamic_input(
            SYSTEM, name, EMITTER_UPDATE, NODES[name]["beam"], endpoint, feeder)))


def attach_rim(name):
    cdo = builder()
    for endpoint, _, vector_input, anchored in ENDPOINTS:
        if anchored:
            record("%s %s anchor" % (name, endpoint), cdo.set_input_dynamic_input(
                SYSTEM, name, EMITTER_UPDATE, NODES[name][endpoint], "Position", DYN_SIMULATION_POSITION))
        NODES[name][endpoint + " rim"] = record("%s %s rim" % (name, endpoint), str(
            cdo.set_input_dynamic_input(SYSTEM, name, EMITTER_UPDATE, NODES[name][endpoint], vector_input,
                                        DYN_RANDOM_VECTOR)))


def attach_scales(name):
    """Zeroing the Z component of the scale is what flattens a random unit vector onto the camera's plane."""
    cdo = builder()
    for endpoint, _, _, _ in ENDPOINTS:
        NODES[name][endpoint + " scale"] = record("%s %s scale" % (name, endpoint), str(
            cdo.set_input_dynamic_input(SYSTEM, name, EMITTER_UPDATE, NODES[name][endpoint + " rim"],
                                        "Vector Scale", DYN_MULTIPLY_VECTOR_BY_FLOAT)))


def set_values(layer):
    name, rim, points, loop, odds, lifetime, width, color, jag, curl, jitter = layer
    cdo = builder()
    nodes = NODES[name]

    def write(usage, node, input_name, value):
        record("%s %s %s" % (name, node, input_name),
               cdo.set_input_value(SYSTEM, name, usage, node, input_name, str(value)))

    def flat(value):
        """A three component input reaches out of the camera's plane, so its Z always stays zero."""
        return "%s,%s,0" % (value, value)

    record("%s loop behavior" % name,
           cdo.set_static_switch(SYSTEM, name, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite"))
    write(EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", loop)
    for input_name, value in (("Spawn Count", points), ("Spawn Probability", odds), ("Spawn Time", 0),
                              ("Loop Count Limit", 0)):
        write(EMITTER_UPDATE, BURST, input_name, value)
    for endpoint, _, _, _ in ENDPOINTS:
        write(EMITTER_UPDATE, nodes[endpoint + " scale"], "Vector", flat(rim))
        record("%s %s radius" % (name, endpoint), cdo.set_input_linked_parameter(
            SYSTEM, name, EMITTER_UPDATE, nodes[endpoint + " scale"], "Float", "User.Radius"))

    record("%s lifetime mode" % name, cdo.set_static_switch(
        SYSTEM, name, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime Mode", "Direct Set"))
    write(PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", lifetime)
    write(PARTICLE_SPAWN, BEAM_WIDTH_CURVE, "Scale Curve", width)
    write(PARTICLE_SPAWN, nodes["jag"], "Noise Strength", flat(jag))
    write(PARTICLE_SPAWN, nodes["jag"], "Noise Frequency", flat(JAG_FREQUENCY))

    write(PARTICLE_UPDATE, nodes["curl"], "Noise Strength", curl)
    write(PARTICLE_UPDATE, nodes["curl"], "Noise Frequency", CURL_FREQUENCY)
    write(PARTICLE_UPDATE, nodes["drag"], "Drag", DRAG)
    write(PARTICLE_UPDATE, nodes["jitter"], "Jitter Amount", jitter)
    write(PARTICLE_UPDATE, nodes["jitter"], "Jitter Delay", JITTER_DELAY)
    write(PARTICLE_UPDATE, COLOR, "Color", color)


def renderer(name):
    """Additive ribbon, corners left sharp — a smoothed bolt reads as a noodle."""
    emitter = unreal.find_object(unreal.load_object(None, SYSTEM), name)
    ribbon = unreal.find_object(emitter, "NiagaraRibbonRendererProperties_0") if emitter else None
    if not ribbon:
        record("%s renderer" % name, "NOT FOUND")
        return
    ribbon.set_editor_property("Material", unreal.load_object(None, RIBBON_MATERIAL))
    ribbon.set_editor_property("CurveTension", CURVE_TENSION)
    record("%s renderer" % name, ribbon.get_editor_property("Material").get_name())


def main():
    """Each phase runs across every layer before the compile that makes its new nodes addressable."""
    cdo = builder()
    add_layer_emitters()
    record("User.Radius", cdo.set_user_parameter(SYSTEM, "User.Radius", str(AURA_RADIUS)))
    for phase in (add_modules, attach_endpoints, attach_rim, attach_scales):
        for layer in LAYERS:
            phase(layer[0])
        record("compile after %s" % phase.__name__, cdo.compile_and_save(SYSTEM))
    for layer in LAYERS:
        set_values(layer)
        renderer(layer[0])
    record("compile", cdo.compile_and_save(SYSTEM))
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_StaticElectricity.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(main())
