"""An electric trail left behind a damage-boosted shot.

Builds /Game/Art/VFX/Generic/Niagara/NS_ChargedTrail, worn by any projectile whose shooter carries a damage
boost — UGeoFXComponent attaches it to the projectile's root, so every layer here is anchored on a body moving
at GeneralSpellSpeed and nothing has to be told where the shot is.

That movement is the whole mechanism. Both layers simulate in world space, so a particle stays where it was
born while the shot runs on, and the path the shot took is what the effect draws:

  layer   renderer  placement                 motion                    emission
  Trail   ribbon    scattered on the shot     curl noise vs drag        continuous rate
  Arc     ribbon    beam across the shot      curl noise vs drag        restriking bursts

Trail is the strand. Particles are born at the shot and linked in birth order, so the ribbon traces the flight
path on its own — the coherent shape comes from the shot's motion, not from a force. What makes it read as
electricity rather than as a glowing wake is that ShapeLocation scatters each birth on a small sphere whose
radius is close to the gap the shot covers between two spawns: consecutive points step sideways about as far
as they step forward, so the strand is born zigzagged. Curl noise against drag then bends what is already laid,
and since a curl field is spatially coherent the whole tail writhes as one shape while it fades.

Both the alpha and the ribbon width fall off with age, so the strand tapers away behind the shot instead of
ending on a cut edge.

Arc is the discharge. Its bolts strike between two points re-rolled every frame around the shot, on a loop
carrying a spawn probability so they never fall into a beat, and each bolt is left where it struck — a run of
strikes strings itself along the trail without any of them knowing where the trail is.

Two layers rather than the three the character auras carry: a boosted volley puts one of these on every shot in
the air, so the cost is paid per projectile rather than per character.

`User.Radius` is the one number both layers are fitted from — the shot's radius, not a character's. Nothing
writes it at runtime (a buff system is spawned with no parameters), so it is authored to the project's default
projectile radius. `User.Lifetime` is inherited from the base system and drives nothing.

A run is `prepare()`, then the `niagara_ops` route to drop the base system's own emitter — the one step no
Python here reaches — then `main()`.

Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
TARGET_NAME = "NS_ChargedTrail"
SYSTEM = "%s/%s.%s" % (TARGET_FOLDER, TARGET_NAME, TARGET_NAME)
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_CircleArround.NS_CircleArround"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_LayerSources"
RIBBON_MATERIAL = "/Niagara/DefaultAssets/DefaultRIbbonMaterial.DefaultRibbonMaterial"

TEMPLATE_RIBBON = "/Niagara/DefaultAssets/Templates/Emitters/LocationBasedRibbon.LocationBasedRibbon"
TEMPLATE_BEAM = "/Niagara/DefaultAssets/Templates/Emitters/StaticBeam.StaticBeam"

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"
BEAM_SETUP = "BeamEmitterSetup"
BEAM_WIDTH_CURVE = "FloatFromCurve002"
COLOR = "Color"

MODULE_SPAWN_RATE = "/Niagara/Modules/Emitter/SpawnRate.SpawnRate"
MODULE_SHAPE_LOCATION = "/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"
MODULE_BEAM_SETUP = "/Niagara/Modules/Beams/BeamEmitterSetup.BeamEmitterSetup"
MODULE_CURL_NOISE_LOCATION = "/Niagara/Modules/Spawn/Location/CurlNoiseLocation.CurlNoiseLocation"
MODULE_CURL_NOISE_FORCE = "/Niagara/Modules/Update/Forces/CurlNoiseForce.CurlNoiseForce"
MODULE_DRAG = "/Niagara/Modules/Update/Forces/Drag.Drag"
MODULE_JITTER_POSITION = "/Niagara/Modules/Update/Position/JitterPosition.JitterPosition"
MODULE_SCALE_COLOR = "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"
MODULE_SCALE_RIBBON_WIDTH = "/Niagara/Modules/Ribbons/ScaleRibbonWidth.ScaleRibbonWidth"

DYN_ADD_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Vectors/Position/AddVectorToPosition.AddVectorToPosition"
DYN_CONVERT_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Transforms/ConvertVectorToPosition.ConvertVectorToPosition"
DYN_SIMULATION_POSITION = "/Niagara/DynamicInputs/Helpers/SimulationPosition.SimulationPosition"
DYN_RANDOM_VECTOR = "/Niagara/DynamicInputs/Random/V2/RandomVector.RandomVector"
DYN_MULTIPLY_VECTOR_BY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat"
DYN_MULTIPLY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_Float.Multiply_Float"
DYN_ONE_MINUS_FLOAT = "/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat"

TRAIL = "Trail"
ARC = "Arc"

SHOT_RADIUS = 30.0  # UGameDataSettings::GeneralProjectileRadius, the shot this dresses
CURVE_TENSION = 0.99  # tension IS sharpness: a slack ribbon rounds its corners off and reads as a noodle
CURL_FREQUENCY = 0.03  # one feature spans the whole effect, so a strand bends as one piece
DRAG = 6.0  # against a curl force this settles a terminal speed, which is what bounds the bend

# The scatter is sized against the gap the shot covers between two spawns (speed / rate), not picked for its
# own sake: sideways steps shorter than the forward ones leave the strand smooth however random they are.
TRAIL_RATE = 90.0
TRAIL_LIFETIME = 0.28
TRAIL_SCATTER = 0.30  # of User.Radius, ~9uu against the ~13uu a 1200uu/s shot covers per spawn
TRAIL_WIDTH = 7.0
TRAIL_COLOR = "2.00,0.15,6.50,1"  # the DamageBoost palette hue, pushed into HDR for an additive ribbon
TRAIL_CURL = 500.0

ARC_LOOP = 0.13
ARC_ODDS = 0.70
ARC_POINTS = 12
ARC_LIFETIME = 0.09
ARC_REACH = 1.25  # of User.Radius: the bolts crackle just clear of the shot
ARC_WIDTH = 2.2
ARC_COLOR = "5.00,2.00,9.00,1"
ARC_JAG = 5.0
ARC_JAG_FREQUENCY = 0.25  # a feature smaller than the gap between two beam points, or the strand only shifts
ARC_CURL = 300.0
ARC_JITTER = 1.6
ARC_JITTER_DELAY = 0.03

# Input name, the node feeding it, that node's own vector input, and whether it carries the emitter position.
# The two endpoints are not symmetric: the start is an absolute position, so the emitter's own has to be added
# under it, while the end is an offset the module adds that same position to.
ENDPOINTS = (
    ("Beam Start", DYN_ADD_VECTOR_TO_POSITION, "Vector", True),
    ("Beam End", DYN_CONVERT_VECTOR_TO_POSITION, "Input Position", False),
)

NODES = {TRAIL: {}, ARC: {}}
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
    """Rewrites the target and both layer sources; the base system's own emitter goes next."""
    duplicate_into(BASE_SYSTEM, TARGET_FOLDER, TARGET_NAME)
    duplicate_into(TEMPLATE_RIBBON, LAYER_SOURCE_FOLDER, TRAIL)
    duplicate_into(TEMPLATE_BEAM, LAYER_SOURCE_FOLDER, ARC)
    return "\n".join(LOG)


def write(layer, usage, node, input_name, value):
    record("%s %s %s" % (layer, node, input_name),
           builder().set_input_value(SYSTEM, layer, usage, node, input_name, str(value)))


def switch(layer, usage, node, switch_name, entry):
    record("%s %s %s" % (layer, node, switch_name),
           builder().set_static_switch(SYSTEM, layer, usage, node, switch_name, entry))


def nest(layer, usage, node, input_name, script):
    return record("%s %s %s" % (layer, node, input_name), str(
        builder().set_input_dynamic_input(SYSTEM, layer, usage, node, input_name, script)))


def link(layer, usage, node, input_name, parameter):
    record("%s %s %s" % (layer, node, input_name),
           builder().set_input_linked_parameter(SYSTEM, layer, usage, node, input_name, parameter))


def flat(value):
    """A three component input reaches out of the camera's plane, so its Z always stays zero."""
    return "%s,%s,0" % (value, value)


def add_emitters():
    for layer in (TRAIL, ARC):
        record("emitter %s" % layer,
               builder().add_emitter(SYSTEM, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, layer, layer)))
    record("User.Radius", builder().set_user_parameter(SYSTEM, "User.Radius", str(SHOT_RADIUS)))


def add_modules():
    """Forces go above the solver that reads them; everything else appends."""
    cdo = builder()
    NODES[TRAIL] = {
        "rate": str(cdo.add_module(SYSTEM, TRAIL, EMITTER_UPDATE, MODULE_SPAWN_RATE)),
        "scatter": str(cdo.add_module(SYSTEM, TRAIL, PARTICLE_SPAWN, MODULE_SHAPE_LOCATION)),
        "curl": str(cdo.add_module(SYSTEM, TRAIL, PARTICLE_UPDATE, MODULE_CURL_NOISE_FORCE, 0)),
        "drag": str(cdo.add_module(SYSTEM, TRAIL, PARTICLE_UPDATE, MODULE_DRAG, 1)),
        "fade": str(cdo.add_module(SYSTEM, TRAIL, PARTICLE_UPDATE, MODULE_SCALE_COLOR)),
        "taper": str(cdo.add_module(SYSTEM, TRAIL, PARTICLE_UPDATE, MODULE_SCALE_RIBBON_WIDTH)),
    }
    # A dynamic input attaches only where the stack holds a plain value, and the template already feeds its
    # beam start, so the template's own setup is disabled and a second one built on instead.
    record("%s disable template beam" % ARC,
           cdo.set_module_enabled(SYSTEM, ARC, EMITTER_UPDATE, BEAM_SETUP, False))
    NODES[ARC] = {
        "beam": str(cdo.add_module(SYSTEM, ARC, EMITTER_UPDATE, MODULE_BEAM_SETUP)),
        "jag": str(cdo.add_module(SYSTEM, ARC, PARTICLE_SPAWN, MODULE_CURL_NOISE_LOCATION)),
        "curl": str(cdo.add_module(SYSTEM, ARC, PARTICLE_UPDATE, MODULE_CURL_NOISE_FORCE, 0)),
        "drag": str(cdo.add_module(SYSTEM, ARC, PARTICLE_UPDATE, MODULE_DRAG, 1)),
        "jitter": str(cdo.add_module(SYSTEM, ARC, PARTICLE_UPDATE, MODULE_JITTER_POSITION)),
    }
    for layer, nodes in NODES.items():
        record("%s modules" % layer, nodes)


def set_switches():
    """Every branch the values below live on; a switch changes which inputs exist, so it precedes them."""
    for layer in (TRAIL, ARC):
        switch(layer, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
        switch(layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime Mode", "Direct Set")
    switch(TRAIL, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color Mode", "Direct Set")
    switch(TRAIL, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Ribbon Width Mode", "Direct Set")


def attach_first():
    NODES[TRAIL]["scatter radius"] = nest(TRAIL, PARTICLE_SPAWN, NODES[TRAIL]["scatter"], "Sphere Radius",
                                          DYN_MULTIPLY_FLOAT)
    NODES[TRAIL]["alpha"] = nest(TRAIL, PARTICLE_UPDATE, NODES[TRAIL]["fade"], "Scale Alpha",
                                 DYN_ONE_MINUS_FLOAT)
    NODES[TRAIL]["width"] = nest(TRAIL, PARTICLE_UPDATE, NODES[TRAIL]["taper"], "Ribbon Width Scale",
                                 DYN_ONE_MINUS_FLOAT)
    for endpoint, feeder, _, _ in ENDPOINTS:
        NODES[ARC][endpoint] = nest(ARC, EMITTER_UPDATE, NODES[ARC]["beam"], endpoint, feeder)


def attach_second():
    for endpoint, _, vector_input, anchored in ENDPOINTS:
        if anchored:
            record("%s %s anchor" % (ARC, endpoint), builder().set_input_dynamic_input(
                SYSTEM, ARC, EMITTER_UPDATE, NODES[ARC][endpoint], "Position", DYN_SIMULATION_POSITION))
        NODES[ARC][endpoint + " reach"] = nest(ARC, EMITTER_UPDATE, NODES[ARC][endpoint], vector_input,
                                               DYN_RANDOM_VECTOR)


def attach_third():
    """Zeroing the Z of the scale is what flattens a random unit vector onto the camera's plane."""
    for endpoint, _, _, _ in ENDPOINTS:
        NODES[ARC][endpoint + " scale"] = nest(ARC, EMITTER_UPDATE, NODES[ARC][endpoint + " reach"],
                                               "Vector Scale", DYN_MULTIPLY_VECTOR_BY_FLOAT)


def set_trail_values():
    write(TRAIL, EMITTER_UPDATE, NODES[TRAIL]["rate"], "SpawnRate", TRAIL_RATE)
    write(TRAIL, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", TRAIL_LIFETIME)
    write(TRAIL, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", TRAIL_COLOR)
    write(TRAIL, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Ribbon Width", TRAIL_WIDTH)
    # On the surface rather than through the volume: a point drawn near the centre would fall back onto the
    # flight path and flatten that step of the zigzag.
    write(TRAIL, PARTICLE_SPAWN, NODES[TRAIL]["scatter"], "Sphere Surface Distribution", 1.0)
    link(TRAIL, PARTICLE_SPAWN, NODES[TRAIL]["scatter radius"], "A", "User.Radius")
    write(TRAIL, PARTICLE_SPAWN, NODES[TRAIL]["scatter radius"], "B", TRAIL_SCATTER)
    write(TRAIL, PARTICLE_UPDATE, NODES[TRAIL]["curl"], "Noise Strength", TRAIL_CURL)
    write(TRAIL, PARTICLE_UPDATE, NODES[TRAIL]["curl"], "Noise Frequency", CURL_FREQUENCY)
    write(TRAIL, PARTICLE_UPDATE, NODES[TRAIL]["drag"], "Drag", DRAG)
    for node in ("alpha", "width"):
        link(TRAIL, PARTICLE_UPDATE, NODES[TRAIL][node], "Float", "Particles.NormalizedAge")


def set_arc_values():
    write(ARC, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", ARC_LOOP)
    for input_name, value in (("Spawn Count", ARC_POINTS), ("Spawn Probability", ARC_ODDS),
                              ("Spawn Time", 0), ("Loop Count Limit", 0)):
        write(ARC, EMITTER_UPDATE, BURST, input_name, value)
    for endpoint, _, _, _ in ENDPOINTS:
        write(ARC, EMITTER_UPDATE, NODES[ARC][endpoint + " scale"], "Vector", flat(ARC_REACH))
        link(ARC, EMITTER_UPDATE, NODES[ARC][endpoint + " scale"], "Float", "User.Radius")

    write(ARC, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", ARC_LIFETIME)
    write(ARC, PARTICLE_SPAWN, BEAM_WIDTH_CURVE, "Scale Curve", ARC_WIDTH)
    write(ARC, PARTICLE_SPAWN, NODES[ARC]["jag"], "Noise Strength", flat(ARC_JAG))
    write(ARC, PARTICLE_SPAWN, NODES[ARC]["jag"], "Noise Frequency", flat(ARC_JAG_FREQUENCY))
    write(ARC, PARTICLE_UPDATE, COLOR, "Color", ARC_COLOR)
    write(ARC, PARTICLE_UPDATE, NODES[ARC]["curl"], "Noise Strength", ARC_CURL)
    write(ARC, PARTICLE_UPDATE, NODES[ARC]["curl"], "Noise Frequency", CURL_FREQUENCY)
    write(ARC, PARTICLE_UPDATE, NODES[ARC]["drag"], "Drag", DRAG)
    write(ARC, PARTICLE_UPDATE, NODES[ARC]["jitter"], "Jitter Amount", ARC_JITTER)
    write(ARC, PARTICLE_UPDATE, NODES[ARC]["jitter"], "Jitter Delay", ARC_JITTER_DELAY)


def renderer(layer):
    """Additive ribbon, corners left sharp — a smoothed bolt reads as a noodle."""
    emitter = unreal.find_object(unreal.load_object(None, SYSTEM), layer)
    ribbon = unreal.find_object(emitter, "NiagaraRibbonRendererProperties_0") if emitter else None
    if not ribbon:
        record("%s renderer" % layer, "NOT FOUND")
        return
    ribbon.set_editor_property("Material", unreal.load_object(None, RIBBON_MATERIAL))
    ribbon.set_editor_property("CurveTension", CURVE_TENSION)
    record("%s renderer" % layer, ribbon.get_editor_property("Material").get_name())


def main():
    """Each phase runs before the compile that makes the nodes it added addressable by value."""
    cdo = builder()
    add_emitters()
    for phase in (add_modules, set_switches, attach_first, attach_second, attach_third):
        phase()
        record("compile after %s" % phase.__name__, cdo.compile_and_save(SYSTEM))
    set_trail_values()
    set_arc_values()
    for layer in (TRAIL, ARC):
        renderer(layer)
    record("compile", cdo.compile_and_save(SYSTEM))
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_ChargedTrail.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(main())
