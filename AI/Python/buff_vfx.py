"""Every buff a character or its shots can wear, in one style.

Builds the six systems named by UGameDataSettings::BuffVFX, into /Game/Art/VFX/Generic/Niagara:

  system            attribute                 worn by  the strand is        emission
  NS_ChargedTrail   DamageMultiplier          the shot the path travelled   rate plus restriking bursts
  NS_VitalTrail     AppliedHealBoost          the shot the path travelled   continuous rate
  NS_SwiftWake      MovementSpeedMultiplier   the body the path travelled   continuous rate
  NS_ChargedHalo    DamageMultiplier          the body a chord that circles one held strand plus bursts
  NS_VitalHalo      AppliedHealBoost          the body a chord that circles two held strands
  NS_MendingDrift   ReceivedHealBoost         the body no strand — sprites  continuous rate
  NS_BulwarkShell   DamageReduction           the body no strand — sprites  continuous rate

NS_ChargedTrail is built by AI/Python/charged_trail_vfx.py and is not rewritten here; it is the shape the
other ribbons follow.

A strand is drawn one of two ways, and which one a system uses follows from whether its owner moves.

A shot moves, so its strand is simply where it has been: particles are born on it, left where they were born,
and linked in birth order, so the ribbon *is* the flight path. The sphere the births are scattered on is then
the whole character of it — scattered wider than the gap the shot covers between two births, consecutive
points step sideways as far as they step forward and the strand is born zigzagged; scattered under it, it
stays smooth. A curl noise force bends what is already laid, and because a curl field is spatially coherent
the strand can only ever bend as one shape: how many of the field's features span its length is what decides
between a lean and an ondulation, and two layers given the same field at opposite strengths bend into mirror
images of each other, which is what makes them braid. NS_SwiftWake is that same trail worn by a body, so it
draws a contrail while its owner runs and collapses to a knot when it stops — which is exactly when a
movement-speed buff has nothing to say.

A body standing still has no path to lend, so a halo makes one out of a beam instead: two endpoints circling
it on unrelated periods, and UpdateBeam re-deriving every particle from them each frame, so the chord between
them sweeps and stretches without ever settling. This is the one mechanism a body's own motion cannot break,
and it is what NS_ChargedHalo and NS_VitalHalo are built on — the first jagged and restriking over the top,
the second two slack strands crossing each other.

The two sprite systems answer buffs that never reach a projectile, so neither needs a strand: motes drawn
inward for healing received, and streaks orbiting the rim for damage reduced. Both emit at a continuous rate,
which reads as a state; only NS_ChargedHalo's bolts burst, and they are the same restriking layer the shot
wears.

Every system carries `User.Radius`, the body or shot it is fitted to. Nothing writes it at runtime — a buff
system is spawned with no parameters — so it is authored to the default each side has.

A run is `prepare()`, then the `niagara_ops` route to drop each base system's own emitter — the one step no
Python here reaches — then `main()`.

Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import math

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_CircleArround.NS_CircleArround"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_LayerSources"
RIBBON_MATERIAL = "/Niagara/DefaultAssets/DefaultRIbbonMaterial.DefaultRibbonMaterial"
GLOW_MATERIAL = "/Game/Art/VFX/Generic/Materials/MatInstances/MI_Glow01.MI_Glow01"

TEMPLATE_RIBBON = "/Niagara/DefaultAssets/Templates/Emitters/LocationBasedRibbon.LocationBasedRibbon"
TEMPLATE_BEAM = "/Niagara/DefaultAssets/Templates/Emitters/StaticBeam.StaticBeam"
TEMPLATE_SPRITE = "/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"
BEAM_SETUP = "BeamEmitterSetup"
BEAM_WIDTH_CURVE = "FloatFromCurve002"
COLOR = "Color"
TEMPLATE_FADE = "ScaleColor"  # SimpleSpriteBurst's own, which holds alpha at ~0 at every age

MODULE_SPAWN_RATE = "/Niagara/Modules/Emitter/SpawnRate.SpawnRate"
MODULE_SHAPE_LOCATION = "/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"
MODULE_BEAM_SETUP = "/Niagara/Modules/Beams/BeamEmitterSetup.BeamEmitterSetup"
MODULE_UPDATE_BEAM = "/Niagara/Modules/Beams/UpdateBeam.UpdateBeam"
MODULE_CURL_NOISE_LOCATION = "/Niagara/Modules/Spawn/Location/CurlNoiseLocation.CurlNoiseLocation"
MODULE_CURL_NOISE_FORCE = "/Niagara/Modules/Update/Forces/CurlNoiseForce.CurlNoiseForce"
MODULE_DRAG = "/Niagara/Modules/Update/Forces/Drag.Drag"
MODULE_JITTER_POSITION = "/Niagara/Modules/Update/Position/JitterPosition.JitterPosition"
MODULE_POINT_ATTRACTION = "/Niagara/Modules/Update/Forces/PointAttractionForce.PointAttractionForce"
MODULE_VORTEX_VELOCITY = "/Niagara/Modules/Update/Velocity/VortexVelocity.VortexVelocity"
MODULE_SCALE_COLOR = "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"
MODULE_SCALE_RIBBON_WIDTH = "/Niagara/Modules/Ribbons/ScaleRibbonWidth.ScaleRibbonWidth"

DYN_ADD_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Vectors/Position/AddVectorToPosition.AddVectorToPosition"
DYN_CONVERT_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Transforms/ConvertVectorToPosition.ConvertVectorToPosition"
DYN_SIMULATION_POSITION = "/Niagara/DynamicInputs/Helpers/SimulationPosition.SimulationPosition"
DYN_RANDOM_VECTOR = "/Niagara/DynamicInputs/Random/V2/RandomVector.RandomVector"
DYN_MAKE_VECTOR = "/Niagara/DynamicInputs/TypeConversions/MakeVector.MakeVector"
DYN_COSINE = "/Niagara/DynamicInputs/Angles/Cosine.Cosine"
DYN_SINE = "/Niagara/DynamicInputs/Angles/Sine.Sine"
DYN_MULTIPLY_VECTOR_BY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat"
DYN_MULTIPLY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_Float.Multiply_Float"
DYN_ONE_MINUS_FLOAT = "/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat"

BODY_RADIUS = 50.0  # the playable character's capsule radius
SHOT_RADIUS = 30.0  # UGameDataSettings::GeneralProjectileRadius

SHARP = 0.99  # ribbon tension IS sharpness: lightning wants the corners it is born with
SLACK = 0.20  # and a smooth strand wants them rounded off

ZIGZAG = 0.25  # a jag feature smaller than the gap between two beam points, or the strand only shifts
# The force's own frequency is what a strand reads as. Neighbours a few uu apart always sample nearly the
# same value, so the whole strand only ever bends as one shape; how much of that shape fits in the strand's
# own length is the choice. One feature over the effect leans it, four along it make it undulate.
WHOLE = 0.03
WAVE = 0.012

# Input name, the node feeding it, that node's own vector input, and whether it carries the emitter position.
# The two endpoints are not symmetric: the start is an absolute position, so the emitter's own has to be added
# under it, while the end is an offset the module adds that same position to.
ENDPOINTS = (
    ("Beam Start", DYN_ADD_VECTOR_TO_POSITION, "Vector", True),
    ("Beam End", DYN_CONVERT_VECTOR_TO_POSITION, "Input Position", False),
)


def strand(name, color, width, rate, lifetime, curl, frequency, drag, tension,
           scatter=0.0, orbit=0.0, orbit_radius=0.0, jitter=0.0):
    """A ribbon spread by particle age: orbit degrees a second about the owner, or 0 to be left on its path."""
    return {"kind": "strand", "name": name, "template": TEMPLATE_RIBBON, "color": color, "width": width,
            "rate": rate, "lifetime": lifetime, "curl": curl, "frequency": frequency, "drag": drag,
            "tension": tension, "scatter": scatter, "orbit": orbit, "orbit_radius": orbit_radius,
            "jitter": jitter}


def bolt(name, color, width, loop, odds, points, lifetime, reach=0.0, periods=(),
         jag=5.0, jitter=1.6, jitter_delay=0.03, tension=SHARP):
    """A strand held between two points beside the owner: re-rolled every strike, or circling it forever."""
    return {"kind": "bolt", "name": name, "template": TEMPLATE_BEAM, "color": color, "width": width,
            "loop": loop, "odds": odds, "points": points, "lifetime": lifetime, "reach": reach,
            "periods": periods, "jag": jag, "jitter": jitter, "jitter_delay": jitter_delay,
            "tension": tension}


def motes(name, color, size, rate, lifetime, ring, pull=(), spin=()):
    """Sprites off the rim; a two component size is a streak, which is drawn along its own travel."""
    return {"kind": "motes", "name": name, "template": TEMPLATE_SPRITE, "color": color, "size": size,
            "rate": rate, "lifetime": lifetime, "ring": ring, "pull": pull, "spin": spin}


# The shot's own trail already exists as NS_ChargedTrail; every strand below is that one re-aimed.
SYSTEMS = (
    ("NS_VitalTrail", SHOT_RADIUS, (
        strand("HealWave", "2.60,6.50,0.40,1", 9.0, 110.0, 0.40, 500.0, WAVE, 4.0, SLACK, scatter=0.05),
        strand("HealBraid", "4.20,9.00,1.20,1", 5.5, 110.0, 0.40, -500.0, WAVE, 4.0, SLACK, scatter=0.05),
    )),
    ("NS_ChargedHalo", BODY_RADIUS, (
        bolt("ChargeRing", "2.00,0.15,6.50,1", 4.0, 2.00, 1.00, 26, 1.80, periods=(0.9, -1.3), jitter=2.5),
        bolt("ChargeSnap", "5.00,2.00,9.00,1", 2.2, 0.13, 0.70, 12, 0.09, reach=1.25),
    )),
    # Three strands on unrelated periods: with only two, both are near the same place whenever their phases
    # happen to agree and the aura thins out to one sliver.
    ("NS_VitalHalo", BODY_RADIUS, (
        bolt("HealWeave", "2.20,5.50,0.35,1", 11.0, 2.60, 1.00, 30, 2.40, periods=(2.6, -3.4),
             jitter=1.2, jitter_delay=0.12, tension=SLACK),
        bolt("HealCounter", "3.80,8.50,1.00,1", 7.0, 2.60, 1.00, 30, 2.40, periods=(-3.1, 2.2),
             jitter=1.2, jitter_delay=0.12, tension=SLACK),
        bolt("HealDrift", "1.60,4.00,0.20,1", 14.0, 3.40, 1.00, 30, 3.20, periods=(4.3, 5.9),
             jitter=0.8, jitter_delay=0.20, tension=SLACK),
    )),
    ("NS_SwiftWake", BODY_RADIUS, (
        strand("RushWake", "7.00,3.60,0.30,1", 7.0, 130.0, 0.30, 180.0, WAVE, 3.0, SLACK, scatter=0.30),
        strand("RushCounter", "9.00,5.20,0.60,1", 4.5, 130.0, 0.30, -180.0, WAVE, 3.0, SLACK, scatter=0.30),
    )),
    ("NS_MendingDrift", BODY_RADIUS, (
        motes("MendDraw", "0.60,5.50,1.20,1", 7.0, 70.0, 0.75, 1.90, pull=(260.0, 220.0, 2.0)),
    )),
    ("NS_BulwarkShell", BODY_RADIUS, (
        motes("GuardPlate", "0.35,0.90,7.00,1", "22,2.6", 110.0, 0.90, 1.05, spin=(300.0, 150.0)),
    )),
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


def system_path(system_name):
    return "%s/%s.%s" % (TARGET_FOLDER, system_name, system_name)


def prepare():
    """Rewrites every target and every layer source; dropping each base's own emitter goes next."""
    for system_name, _, layers in SYSTEMS:
        duplicate_into(BASE_SYSTEM, TARGET_FOLDER, system_name)
        for layer in layers:
            duplicate_into(layer["template"], LAYER_SOURCE_FOLDER, layer["name"])
    return "\n".join(LOG)


def write(system, layer, usage, node, input_name, value):
    record("%s %s %s" % (layer["name"], node, input_name),
           builder().set_input_value(system, layer["name"], usage, node, input_name, str(value)))


def switch(system, layer, usage, node, switch_name, entry):
    record("%s %s %s" % (layer["name"], node, switch_name),
           builder().set_static_switch(system, layer["name"], usage, node, switch_name, entry))


def nest(system, layer, usage, node, input_name, script):
    return record("%s %s %s" % (layer["name"], node, input_name), str(
        builder().set_input_dynamic_input(system, layer["name"], usage, node, input_name, script)))


def link(system, layer, usage, node, input_name, parameter):
    record("%s %s %s" % (layer["name"], node, input_name),
           builder().set_input_linked_parameter(system, layer["name"], usage, node, input_name, parameter))


def flat(value):
    """A three component input reaches out of the camera's plane, so its Z always stays zero."""
    return "%s,%s,0" % (value, value)


def nodes(layer):
    return NODES[layer["name"]]


# --- strand ----------------------------------------------------------------------------------------------

def add_strand(system, layer):
    """Forces go above the solver that reads them; everything else appends."""
    cdo = builder()
    name = layer["name"]
    NODES[name] = {
        "rate": str(cdo.add_module(system, name, EMITTER_UPDATE, MODULE_SPAWN_RATE)),
        "curl": str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_CURL_NOISE_FORCE, 0)),
        "drag": str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_DRAG, 1)),
    }
    if layer["orbit"]:
        NODES[name]["orbit"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_VORTEX_VELOCITY, 2))
    else:
        NODES[name]["scatter"] = str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_SHAPE_LOCATION))
    if layer["jitter"]:
        NODES[name]["jitter"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_JITTER_POSITION))
    NODES[name]["fade"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_COLOR))
    NODES[name]["taper"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_RIBBON_WIDTH))

    switch(system, layer, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    for switch_name in ("Lifetime Mode", "Color Mode", "Ribbon Width Mode"):
        switch(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch_name, "Direct Set")


def nest_strand_first(system, layer):
    if not layer["orbit"]:
        nodes(layer)["scatter radius"] = nest(system, layer, PARTICLE_SPAWN, nodes(layer)["scatter"],
                                              "Sphere Radius", DYN_MULTIPLY_FLOAT)
    nodes(layer)["alpha"] = nest(system, layer, PARTICLE_UPDATE, nodes(layer)["fade"], "Scale Alpha",
                                 DYN_ONE_MINUS_FLOAT)
    nodes(layer)["width"] = nest(system, layer, PARTICLE_UPDATE, nodes(layer)["taper"], "Ribbon Width Scale",
                                 DYN_ONE_MINUS_FLOAT)


def set_strand_values(system, layer):
    """One birth point, and age alone spreads the strand off it — around the owner, or behind it."""
    write(system, layer, EMITTER_UPDATE, nodes(layer)["rate"], "SpawnRate", layer["rate"])
    write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", layer["lifetime"])
    write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", layer["color"])
    write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Ribbon Width", layer["width"])

    if layer["orbit"]:
        radius = layer["orbit_radius"] * BODY_RADIUS
        write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Position Offset", "%s,0,0" % radius)
        # A vortex sets a linear speed, and every particle rides the same circle, so the angular rate the
        # strand is authored in becomes that speed once.
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["orbit"], "Velocity Amount",
              math.radians(layer["orbit"]) * radius)
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["orbit"], "Vortex Axis", "0,0,1")
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["orbit"], "Influence Falloff Radius", radius * 4.0)
    else:
        # On the surface rather than through the volume: a point drawn near the centre would fall back onto
        # the path travelled and flatten that step.
        write(system, layer, PARTICLE_SPAWN, nodes(layer)["scatter"], "Sphere Surface Distribution", 1.0)
        link(system, layer, PARTICLE_SPAWN, nodes(layer)["scatter radius"], "A", "User.Radius")
        write(system, layer, PARTICLE_SPAWN, nodes(layer)["scatter radius"], "B", layer["scatter"])

    write(system, layer, PARTICLE_UPDATE, nodes(layer)["curl"], "Noise Strength", layer["curl"])
    write(system, layer, PARTICLE_UPDATE, nodes(layer)["curl"], "Noise Frequency", layer["frequency"])
    write(system, layer, PARTICLE_UPDATE, nodes(layer)["drag"], "Drag", layer["drag"])
    if layer["jitter"]:
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["jitter"], "Jitter Amount", layer["jitter"])
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["jitter"], "Jitter Delay", 0.03)
    for node in ("alpha", "width"):
        link(system, layer, PARTICLE_UPDATE, nodes(layer)[node], "Float", "Particles.NormalizedAge")


# --- bolt ------------------------------------------------------------------------------------------------

def add_bolt(system, layer):
    """A dynamic input attaches only where the stack holds a plain value, and the template already feeds its
    beam start, so the template's own setup is disabled and a second one built on instead."""
    cdo = builder()
    name = layer["name"]
    record("%s disable template beam" % name,
           cdo.set_module_enabled(system, name, EMITTER_UPDATE, BEAM_SETUP, False))
    NODES[name] = {"beam": str(cdo.add_module(system, name, EMITTER_UPDATE, MODULE_BEAM_SETUP))}
    if layer["periods"]:
        # UpdateBeam re-derives every particle from the endpoints each frame, which is what turns two moving
        # endpoints into a sweeping strand — and it has to sit above anything that displaces that strand.
        NODES[name]["follow"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_UPDATE_BEAM, 0))
    else:
        NODES[name]["jag"] = str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_CURL_NOISE_LOCATION))
        NODES[name]["curl"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_CURL_NOISE_FORCE, 0))
        NODES[name]["drag"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_DRAG, 1))
    NODES[name]["jitter"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_JITTER_POSITION))

    switch(system, layer, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    switch(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime Mode", "Direct Set")


def nest_bolt_first(system, layer):
    for endpoint, feeder, _, _ in ENDPOINTS:
        nodes(layer)[endpoint] = nest(system, layer, EMITTER_UPDATE, nodes(layer)["beam"], endpoint, feeder)


def nest_bolt_second(system, layer):
    feeder = DYN_MAKE_VECTOR if layer["periods"] else DYN_RANDOM_VECTOR
    for endpoint, _, vector_input, anchored in ENDPOINTS:
        if anchored:
            record("%s %s anchor" % (layer["name"], endpoint), builder().set_input_dynamic_input(
                system, layer["name"], EMITTER_UPDATE, nodes(layer)[endpoint], "Position",
                DYN_SIMULATION_POSITION))
        nodes(layer)[endpoint + " reach"] = nest(system, layer, EMITTER_UPDATE, nodes(layer)[endpoint],
                                                 vector_input, feeder)


def nest_bolt_third(system, layer):
    """A cosine on X against a sine on Y is a circle, and the trig feeds its own angle from time; a random
    unit vector instead needs its Z zeroed by the scale to stay in the camera's plane."""
    for endpoint, _, _, _ in ENDPOINTS:
        node = nodes(layer)[endpoint + " reach"]
        if layer["periods"]:
            for axis, script in (("X", DYN_COSINE), ("Y", DYN_SINE)):
                nodes(layer)[endpoint + axis] = nest(system, layer, EMITTER_UPDATE, node, axis, script)
        else:
            nodes(layer)[endpoint + " scale"] = nest(system, layer, EMITTER_UPDATE, node, "Vector Scale",
                                                     DYN_MULTIPLY_VECTOR_BY_FLOAT)


def set_bolt_values(system, layer):
    write(system, layer, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", layer["loop"])
    for input_name, value in (("Spawn Count", layer["points"]), ("Spawn Probability", layer["odds"]),
                              ("Spawn Time", 0), ("Loop Count Limit", 0)):
        write(system, layer, EMITTER_UPDATE, BURST, input_name, value)
    # Unrelated periods on the two endpoints keep them from ever lining up, so the strand between them never
    # settles into a fixed length or a fixed angle.
    for endpoint, period in zip((name for name, _, _, _ in ENDPOINTS), layer["periods"]):
        write(system, layer, EMITTER_UPDATE, nodes(layer)[endpoint + " reach"], "Z", 0)
        for axis in ("X", "Y"):
            write(system, layer, EMITTER_UPDATE, nodes(layer)[endpoint + axis], "Period", period)
            link(system, layer, EMITTER_UPDATE, nodes(layer)[endpoint + axis], "Scale", "User.Radius")
    if not layer["periods"]:
        for endpoint, _, _, _ in ENDPOINTS:
            write(system, layer, EMITTER_UPDATE, nodes(layer)[endpoint + " scale"], "Vector",
                  flat(layer["reach"]))
            link(system, layer, EMITTER_UPDATE, nodes(layer)[endpoint + " scale"], "Float", "User.Radius")

    write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", layer["lifetime"])
    write(system, layer, PARTICLE_SPAWN, BEAM_WIDTH_CURVE, "Scale Curve", layer["width"])
    write(system, layer, PARTICLE_UPDATE, COLOR, "Color", layer["color"])
    if not layer["periods"]:
        write(system, layer, PARTICLE_SPAWN, nodes(layer)["jag"], "Noise Strength", flat(layer["jag"]))
        write(system, layer, PARTICLE_SPAWN, nodes(layer)["jag"], "Noise Frequency", flat(ZIGZAG))
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["curl"], "Noise Strength", 300.0)
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["curl"], "Noise Frequency", WHOLE)
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["drag"], "Drag", 6.0)
    write(system, layer, PARTICLE_UPDATE, nodes(layer)["jitter"], "Jitter Amount", layer["jitter"])
    write(system, layer, PARTICLE_UPDATE, nodes(layer)["jitter"], "Jitter Delay", layer["jitter_delay"])


# --- motes -----------------------------------------------------------------------------------------------

def add_motes(system, layer):
    cdo = builder()
    name = layer["name"]
    # A rate reads as a state, so the template's burst goes off; only the ribbon templates carry neither.
    record("%s disable template burst" % name,
           cdo.set_module_enabled(system, name, EMITTER_UPDATE, BURST, False))
    record("%s disable template fade" % name,
           cdo.set_module_enabled(system, name, PARTICLE_UPDATE, TEMPLATE_FADE, False))
    NODES[name] = {
        "rate": str(cdo.add_module(system, name, EMITTER_UPDATE, MODULE_SPAWN_RATE)),
        "place": str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_SHAPE_LOCATION)),
        "fade": str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_COLOR)),
    }
    if layer["pull"]:
        NODES[name]["pull"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_POINT_ATTRACTION, 0))
        NODES[name]["drag"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_DRAG, 1))
    if layer["spin"]:
        NODES[name]["spin"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_VORTEX_VELOCITY, 0))

    switch(system, layer, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    for switch_name, entry in (("Shape Primitive", "Ring / Disc"), ("Ring / Disc Mode", "Circle")):
        switch(system, layer, PARTICLE_SPAWN, nodes(layer)["place"], switch_name, entry)
    for switch_name in ("Lifetime Mode", "Color Mode"):
        switch(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch_name, "Direct Set")
    if not isinstance(layer["size"], float):
        switch(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Sprite Size Mode", "Non-Uniform")


def nest_motes_first(system, layer):
    nodes(layer)["ring"] = nest(system, layer, PARTICLE_SPAWN, nodes(layer)["place"], "Ring Radius",
                                DYN_MULTIPLY_FLOAT)
    nodes(layer)["alpha"] = nest(system, layer, PARTICLE_UPDATE, nodes(layer)["fade"], "Scale Alpha",
                                 DYN_ONE_MINUS_FLOAT)


def set_motes_values(system, layer):
    write(system, layer, EMITTER_UPDATE, nodes(layer)["rate"], "SpawnRate", layer["rate"])
    write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", layer["lifetime"])
    write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", layer["color"])
    if isinstance(layer["size"], float):
        write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", layer["size"])
    else:
        write(system, layer, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Sprite Size", layer["size"])
    link(system, layer, PARTICLE_SPAWN, nodes(layer)["ring"], "A", "User.Radius")
    write(system, layer, PARTICLE_SPAWN, nodes(layer)["ring"], "B", layer["ring"])

    if layer["pull"]:
        strength, reach, drag = layer["pull"]
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["pull"], "AttractionStrength", strength)
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["pull"], "Attraction Radius", reach)
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["drag"], "Drag", drag)
    if layer["spin"]:
        amount, falloff = layer["spin"]
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["spin"], "Velocity Amount", amount)
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["spin"], "Vortex Axis", "0,0,1")
        write(system, layer, PARTICLE_UPDATE, nodes(layer)["spin"], "Influence Falloff Radius", falloff)
    link(system, layer, PARTICLE_UPDATE, nodes(layer)["alpha"], "Float", "Particles.NormalizedAge")


# --- driver ----------------------------------------------------------------------------------------------

RECIPES = {
    "strand": (add_strand, nest_strand_first, None, None, set_strand_values,
               "NiagaraRibbonRendererProperties_0"),
    "bolt": (add_bolt, nest_bolt_first, nest_bolt_second, nest_bolt_third, set_bolt_values,
             "NiagaraRibbonRendererProperties_0"),
    "motes": (add_motes, nest_motes_first, None, None, set_motes_values,
              "NiagaraSpriteRendererProperties_0"),
}


def renderer(system, layer):
    """A streak is drawn along its own travel, which is the sprite renderer's own alignment, not a stack edit."""
    subobject = RECIPES[layer["kind"]][5]
    properties = unreal.find_object(unreal.find_object(unreal.load_object(None, system), layer["name"]),
                                    subobject)
    if not properties:
        record("%s renderer" % layer["name"], "NOT FOUND")
        return
    if layer["kind"] == "motes":
        properties.set_editor_property("Material", unreal.load_object(None, GLOW_MATERIAL))
        if not isinstance(layer["size"], float):
            properties.set_editor_property("Alignment", unreal.NiagaraSpriteAlignment.VELOCITY_ALIGNED)
            properties.set_editor_property("FacingMode", unreal.NiagaraSpriteFacingMode.FACE_CAMERA)
    else:
        properties.set_editor_property("Material", unreal.load_object(None, RIBBON_MATERIAL))
        properties.set_editor_property("CurveTension", layer["tension"])
    record("%s renderer" % layer["name"], subobject)


def build(system_name, radius, layers):
    """Each phase runs before the compile that makes the nodes it added addressable by value."""
    cdo = builder()
    system = system_path(system_name)
    for layer in layers:
        record("%s emitter %s" % (system_name, layer["name"]), cdo.add_emitter(
            system, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, layer["name"], layer["name"])))
    record("%s User.Radius" % system_name, cdo.set_user_parameter(system, "User.Radius", str(radius)))
    record("%s compile emitters" % system_name, cdo.compile_and_save(system))

    for phase in range(4):
        for layer in layers:
            step = RECIPES[layer["kind"]][phase]
            if step:
                step(system, layer)
        record("%s compile phase %d" % (system_name, phase), cdo.compile_and_save(system))

    for layer in layers:
        RECIPES[layer["kind"]][4](system, layer)
        renderer(system, layer)
    record("%s compile" % system_name, cdo.compile_and_save(system))


def main():
    for system_name, radius, layers in SYSTEMS:
        build(system_name, radius, layers)
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_BuffVFX.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(main())
