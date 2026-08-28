"""Coil discharge: an electric helix wound around a cone, turning about its axis.

Builds the emitter stacks of /Game/Art/VFX/Generic/Niagara/NS_ConeCoil.

The sibling effect, NS_StaticElectricity, draws short bolts that restrike between two random rim points. This
one is its opposite in every mechanism, and the two are meant to be read side by side:

  bolt aura                          coil discharge
  a beam between two endpoints       a shape the particles are laid on
  endpoints re-rolled per strike     one strand, held for its whole life
  bent by a noise field              turned rigidly about an axis
  many short strikes                 one long continuous strand

A layer is one strand. Its particles are placed by ShapeLocation's ring, addressed directly, and every input
of that ring is driven by the particle's own normalized execution index: the index winds `U Position` several
turns around the axis, opens `Radius Position` from the centre out to the rim, and lifts the particle along Z.
Radius and height climbing together is what makes the helix a CONE rather than a cylinder — the wider a turn
sits, the higher it sits. Winding on one input while the other two ramp is the whole shape; there is no random
placement anywhere in it.

`VortexVelocity` about Z is the turning. It gives every particle a velocity around the axis rather than a
displacement, so the strand keeps its shape and rotates as one piece — the vortex reads as the coil spinning,
where a noise field would read as the coil fraying. The two layers spin opposite ways, which is what stops the
pair from reading as one rotating object.

What is left of the static crackle is `JitterPosition`: a small per particle offset re-rolled a few times per
life, so the strand shivers on a shape that is otherwise exact.

`User.Radius` scales the whole coil, so it fits any character or boss from one number, and `User.Height` sets
how far it climbs — a height of zero collapses the cone into a flat spiral, which is what an orthographic
camera looking straight down sees anyway.

The base emitter is Niagara's RibbonID behaviour example: the cheapest template carrying a ribbon renderer,
and the one whose ShapeLocation is already in the spawn stack. Its own ribbon id assignment is disabled, since
one strand per emitter is the point.

A run is `prepare()`, then the `niagara_ops` route to drop the base system's own emitter — the one step no
Python here reaches — then `main()`, which adds the layer emitters back and builds their stacks.

Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
TARGET_NAME = "NS_ConeCoil"
SYSTEM = "%s/%s.%s" % (TARGET_FOLDER, TARGET_NAME, TARGET_NAME)
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_CircleArround.NS_CircleArround"
LAYER_TEMPLATE = "/Niagara/DefaultAssets/Templates/BehaviorExamples/RibbonID.RibbonID"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_LayerSources"
RIBBON_MATERIAL = "/Niagara/DefaultAssets/DefaultRIbbonMaterial.DefaultRibbonMaterial"

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

RIBBON_ID_ASSIGNMENT = "SetVariables_142C744B4E42BE9AE994CB8E25D28964"
SHAPE_LOCATION = "ShapeLocation"
INITIALIZE_PARTICLE = "InitializeParticle"
EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"

MODULE_OFFSET_POSITION = "/Niagara/Modules/Update/Position/OffsetPosition.OffsetPosition"
MODULE_VORTEX_VELOCITY = "/Niagara/Modules/Update/Velocity/VortexVelocity.VortexVelocity"
MODULE_JITTER_POSITION = "/Niagara/Modules/Update/Position/JitterPosition.JitterPosition"
MODULE_SCALE_COLOR = "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"

DYN_MULTIPLY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_Float.Multiply_Float"
DYN_MULTIPLY_VECTOR_BY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat"
DYN_ONE_MINUS_FLOAT = "/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat"
DYN_NORMALIZED_EXEC_INDEX = ("/Niagara/DynamicInputs/Execution/ReturnNormalizedExecIndex"
                             ".ReturnNormalizedExecIndex")

COIL_RADIUS = 50.0  # matches the playable character's capsule radius
COIL_HEIGHT = 90.0
VORTEX_AXIS = "0,0,1"  # the coil turns about the axis the orthographic camera looks down
JITTER_DELAY = 0.05
CURVE_TENSION = 0.99  # tension IS sharpness: a slack ribbon rounds its corners off and reads as a noodle

# name, rim radius, height, turns, spin, points, loop seconds, strike odds, lifetime, width, colour, jitter
LAYERS = (
    ("CoilOuter", 1.00, 1.00, 2.5, 150.0, 40, 0.90, 0.85, 0.75, 2.4, "0.45,1.10,3.40,1", 2.0),
    ("CoilInner", 0.55, 0.60, 3.5, -240.0, 28, 0.62, 0.80, 0.52, 1.6, "1.30,1.90,3.20,1", 1.4),
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
    """The lift that turns the spiral into a cone, then the turn, the shiver and the fade."""
    cdo = builder()
    record("%s single ribbon" % name,
           cdo.set_module_enabled(SYSTEM, name, PARTICLE_SPAWN, RIBBON_ID_ASSIGNMENT, False))
    for switch, entry in (("Shape Primitive", "Ring / Disc"), ("Ring / Disc Mode", "Circle"),
                          ("Ring / Disc Distribution Mode", "Direct")):
        record("%s shape %s" % (name, switch),
               cdo.set_static_switch(SYSTEM, name, PARTICLE_SPAWN, SHAPE_LOCATION, switch, entry))
    for switch, entry in (("Lifetime Mode", "Direct Set"), ("Color Mode", "Direct Set"),
                          ("Ribbon Width Mode", "Direct Set"), ("Position Mode", "Simulation Position")):
        record("%s particle %s" % (name, switch),
               cdo.set_static_switch(SYSTEM, name, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch, entry))
    NODES[name] = {
        "lift": str(cdo.add_module(SYSTEM, name, PARTICLE_SPAWN, MODULE_OFFSET_POSITION)),
        "vortex": str(cdo.add_module(SYSTEM, name, PARTICLE_UPDATE, MODULE_VORTEX_VELOCITY)),
        "jitter": str(cdo.add_module(SYSTEM, name, PARTICLE_UPDATE, MODULE_JITTER_POSITION)),
        "fade": str(cdo.add_module(SYSTEM, name, PARTICLE_UPDATE, MODULE_SCALE_COLOR)),
    }
    record("%s modules" % name, NODES[name])


def attach_index(name):
    """Every input of the shape hangs off the same execution index; only the factor under each one differs."""
    cdo = builder()
    for key, usage, node, input_name, feeder in (
            ("radius", PARTICLE_SPAWN, SHAPE_LOCATION, "Ring Radius", DYN_MULTIPLY_FLOAT),
            ("wind", PARTICLE_SPAWN, SHAPE_LOCATION, "U Position", DYN_MULTIPLY_FLOAT),
            ("open", PARTICLE_SPAWN, SHAPE_LOCATION, "Radius Position", DYN_NORMALIZED_EXEC_INDEX),
            ("climb", PARTICLE_SPAWN, NODES[name]["lift"], "Position Offset", DYN_MULTIPLY_VECTOR_BY_FLOAT),
            ("alpha", PARTICLE_UPDATE, NODES[name]["fade"], "Scale Alpha", DYN_ONE_MINUS_FLOAT)):
        NODES[name][key] = record("%s %s" % (name, key), str(cdo.set_input_dynamic_input(
            SYSTEM, name, usage, node, input_name, feeder)))


def attach_index_sources(name):
    cdo = builder()
    for key, input_name in (("wind", "A"), ("climb", "Float")):
        record("%s %s index" % (name, key), cdo.set_input_dynamic_input(
            SYSTEM, name, PARTICLE_SPAWN, NODES[name][key], input_name, DYN_NORMALIZED_EXEC_INDEX))
    record("%s alpha from age" % name, cdo.set_input_linked_parameter(
        SYSTEM, name, PARTICLE_UPDATE, NODES[name]["alpha"], "Float", "Particles.NormalizedAge"))
    record("%s radius from User.Radius" % name, cdo.set_input_linked_parameter(
        SYSTEM, name, PARTICLE_SPAWN, NODES[name]["radius"], "A", "User.Radius"))


def set_values(layer):
    name, rim, height, turns, spin, points, loop, odds, lifetime, width, color, jitter = layer
    cdo = builder()
    nodes = NODES[name]

    def write(usage, node, input_name, value):
        record("%s %s %s" % (name, node, input_name),
               cdo.set_input_value(SYSTEM, name, usage, node, input_name, str(value)))

    record("%s loop behavior" % name,
           cdo.set_static_switch(SYSTEM, name, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite"))
    write(EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", loop)
    for input_name, value in (("Spawn Count", points), ("Spawn Probability", odds), ("Spawn Time", 0),
                              ("Loop Count Limit", 0)):
        write(EMITTER_UPDATE, BURST, input_name, value)

    write(PARTICLE_SPAWN, nodes["radius"], "B", rim)
    write(PARTICLE_SPAWN, nodes["wind"], "B", turns)
    write(PARTICLE_SPAWN, nodes["climb"], "Vector", "0,0,%s" % (COIL_HEIGHT * height))
    for input_name, value in (("Lifetime", lifetime), ("Ribbon Width", width), ("Color", color)):
        write(PARTICLE_SPAWN, INITIALIZE_PARTICLE, input_name, value)

    write(PARTICLE_UPDATE, nodes["vortex"], "Velocity Amount", spin)
    write(PARTICLE_UPDATE, nodes["vortex"], "Vortex Axis", VORTEX_AXIS)
    write(PARTICLE_UPDATE, nodes["jitter"], "Jitter Amount", jitter)
    write(PARTICLE_UPDATE, nodes["jitter"], "Jitter Delay", JITTER_DELAY)


def renderer(name):
    """Additive ribbon, corners left sharp — a smoothed coil reads as a spring rather than a discharge."""
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
    record("User.Radius", cdo.set_user_parameter(SYSTEM, "User.Radius", str(COIL_RADIUS)))
    for phase in (add_modules, attach_index, attach_index_sources):
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
    report = "%sGeoTrinity_ConeCoil.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(main())
