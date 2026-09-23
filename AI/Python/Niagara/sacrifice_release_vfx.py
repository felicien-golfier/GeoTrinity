"""The sacrifice spat back out of the Square badge's keyhole as the detonation ray.

Builds /Game/Art/VFX/Assets/NS_Square_SacrificeRelease, a one-shot made to be spawned on the SKM_SquareBadge
`SacrificeHole` socket facing the aim: it is the reverse of NS_Square_SacrificeHole (sacrifice_hole_vfx.py),
which draws the sacrifice in during the channel.

  layer        reads as
  Core         a thin, very bright line from the hole out to the ray's end
  Halo         a wide dim line of the same hue behind it
  Links        square frames as wide as the ray, lined up along it, shrinking away from the hole outward
  Slugs        solid squares shot out of the hole down the whole length of the ray
  Blast        a square frame bursting out of the hole
  Scatter      eight solid squares thrown off the hole's rim

Local space pointing +X from the socket, lifted above the block's top face like the channel effect. The ray is
`User.Beam_Length` long and `User.Beam_Width` is its half-width, the same pair UGeoBeamVFXComponent writes;
the defaults are GeneralSpellDistance and the detonation's LineHalfWidth. Colour is `User.Color`, multiplied by
each layer's own brightness and re-tinted every frame.

A run is `prepare()`, then the `niagara_ops` route to drop the base system's own emitter (`Explosion`) — the
one step no Python here reaches — then `main()`. The report names every builder call that returned false.

Reference: AI/ArtDirection.md (Rail recipe), AI/MCP/MCP_Niagara.md, AI/VFX.md (Bolt Emitters).
"""

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Assets"
SYSTEM_NAME = "NS_Square_SacrificeRelease"
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_Ray_ZoneIndicator.NS_Ray_ZoneIndicator"  # carries the beam's params
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Assets/_SacrificeReleaseSources"
TEMPLATE_SPRITE = "/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"
TEMPLATE_BEAM = "/Niagara/DefaultAssets/Templates/Emitters/StaticBeam.StaticBeam"
SHAPES = "/Game/Art/VFX/Generic/Materials/MatInstances"
RIBBON_MATERIAL = "/Niagara/DefaultAssets/DefaultRIbbonMaterial.DefaultRibbonMaterial"

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"
TEMPLATE_FADE = "ScaleColor"  # the sprite template's own, which holds alpha at ~0 at every age
BEAM_SETUP = "BeamEmitterSetup"
BEAM_WIDTH_CURVE = "FloatFromCurve002"
BEAM_COLOR = "Color"  # the beam template's own colour module

MODULE_SPAWN_RATE = "/Niagara/Modules/Emitter/SpawnRate.SpawnRate"
MODULE_SHAPE_LOCATION = "/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"
MODULE_VELOCITY = "/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity"
MODULE_VELOCITY_FROM_POINT = "/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint.AddVelocityFromPoint"
MODULE_BEAM_SETUP = "/Niagara/Modules/Beams/BeamEmitterSetup.BeamEmitterSetup"
MODULE_SPRITE_ROTATION_RATE = "/Niagara/Modules/Update/Orientation/SpriteRotationRate.SpriteRotationRate"
MODULE_SCALE_SPRITE_SIZE = "/Niagara/Modules/Update/Size/ScaleSpriteSize.ScaleSpriteSize"
MODULE_SCALE_RIBBON_WIDTH = "/Niagara/Modules/Ribbons/ScaleRibbonWidth.ScaleRibbonWidth"
MODULE_COLOR = "/Niagara/Modules/Update/Color/Color.Color"
MODULE_SCALE_COLOR = "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"

DYN_MULTIPLY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_Float.Multiply_Float"
DYN_SCALE_AND_BIAS = "/Niagara/DynamicInputs/Multiply/ScaleAndBiasFloat.ScaleAndBiasFloat"
DYN_ONE_MINUS_FLOAT = "/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat"
DYN_MULTIPLY_COLOR = "/Niagara/DynamicInputs/Multiply/Multiply_LinearColorByFloat.Multiply_LinearColorByFloat"
DYN_MAKE_VECTOR = "/Niagara/DynamicInputs/TypeConversions/MakeVector.MakeVector"
DYN_NORMALIZED_INDEX = "/Niagara/DynamicInputs/Execution/ReturnNormalizedExecIndex.ReturnNormalizedExecIndex"
DYN_ADD_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Vectors/Position/AddVectorToPosition.AddVectorToPosition"
DYN_CONVERT_VECTOR_TO_POSITION = "/Niagara/DynamicInputs/Transforms/ConvertVectorToPosition.ConvertVectorToPosition"
DYN_SIMULATION_POSITION = "/Niagara/DynamicInputs/Helpers/SimulationPosition.SimulationPosition"

HOLE_RADIUS = 15.0  # SQ_HOLE's radius through the badge's image-to-world scale
LIFT = 55.0  # above the block's top face, measured from the socket at mid height
DEFAULT_COLOR = "0,0,1,0.7"  # the palette's DamageReduction
DEFAULT_LENGTH = 1600.0  # GeneralSpellDistance
DEFAULT_HALF_WIDTH = 100.0  # UGeoSacrificeDetonateAbility::LineHalfWidth
SHARP = 0.99  # ribbon tension IS sharpness: a straight line wants no rounding at all


def beam(name, brightness, width, life, points=32):
    """A straight strand from the hole to the ray's end, narrowing and fading over its life."""
    return {"kind": "beam", "name": name, "template": TEMPLATE_BEAM, "brightness": brightness,
            "width": width, "life": life, "points": points}


def shapes(name, shape, brightness, size, life, count=1, rate=0.0, window=0.0, radius=0.0, lattice=False,
           along=False, width_sized=False, stagger=0.0, spin=0.0, shot=0.0, burst_out=0.0, grow=False,
           shrink=False, fade=False):
    """Sprites of one polygon. `count` bursts once; `rate` spawns for `window` seconds instead.
    `along` lays the burst on a lattice down the ray, `stagger` adds that many seconds of life towards its end,
    `width_sized` makes `size` a multiple of the ray's half-width, `shot` sends each one to the ray's end in
    exactly `life`, and `burst_out` pushes it away from the hole at that speed."""
    return {"kind": "shapes", "name": name, "template": TEMPLATE_SPRITE,
            "shape": "%s/%s.%s" % (SHAPES, shape, shape), "brightness": brightness, "size": size, "life": life,
            "count": count, "rate": rate, "window": window, "radius": radius, "lattice": lattice,
            "along": along, "width_sized": width_sized, "stagger": stagger, "spin": spin, "shot": shot,
            "burst_out": burst_out, "grow": grow, "shrink": shrink, "fade": fade}


LAYERS = (
    beam("Core", 14.0, 6.0, 0.40),
    beam("Halo", 1.2, 60.0, 0.50),
    shapes("Links", "MI_GeoShape_QuadThin", 3.0, 2.0, 0.18, count=9, along=True, width_sized=True,
           stagger=0.40, spin=90.0, shrink=True),
    shapes("Slugs", "MI_GeoShape_QuadSolid", 8.0, 16.0, 0.30, rate=40.0, window=0.15, spin=540.0, shot=1.0),
    shapes("Blast", "MI_GeoShape_QuadThin", 4.0, 90.0, 0.35, spin=-200.0, grow=True, fade=True),
    shapes("Scatter", "MI_GeoShape_QuadSolid", 6.0, 7.0, 0.35, count=8, radius=1.0, lattice=True, spin=400.0,
           burst_out=240.0, shrink=True, fade=True),
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


def system_path():
    return "%s/%s.%s" % (TARGET_FOLDER, SYSTEM_NAME, SYSTEM_NAME)


def prepare_source(one):
    """Renderer and local space go on the layer SOURCE, before it is copied into the system: inside a system
    an emitter's object name is not its handle name, so a lookup by name there can land on another copy."""
    source = duplicate_into(one["template"], LAYER_SOURCE_FOLDER, one["name"])
    if not source:
        return
    if one["kind"] == "beam":
        ribbon = unreal.find_object(source, "NiagaraRibbonRendererProperties_0")
        if ribbon:
            ribbon.set_editor_property("Material", unreal.load_object(None, RIBBON_MATERIAL))
            ribbon.set_editor_property("CurveTension", SHARP)
        record("%s renderer" % one["name"], "ribbon" if ribbon else "NOT FOUND")
    else:
        sprite = unreal.find_object(source, "NiagaraSpriteRendererProperties_0")
        if sprite:
            sprite.set_editor_property("Material", unreal.load_object(None, one["shape"]))
        record("%s renderer" % one["name"], one["shape"].split(".")[-1] if sprite else "NOT FOUND")
    record("%s local space" % one["name"],
           builder().set_emitter_property(source.get_path_name(), "bLocalSpace", "true"))
    unreal.EditorAssetLibrary.save_loaded_asset(source)


def prepare():
    """Rewrites the target and every layer source; dropping the base's own emitter goes next."""
    duplicate_into(BASE_SYSTEM, TARGET_FOLDER, SYSTEM_NAME)
    for one in LAYERS:
        prepare_source(one)
    return "\n".join(LOG)


def write(one, usage, node, input_name, value):
    record("%s %s %s" % (one["name"], node, input_name),
           builder().set_input_value(system_path(), one["name"], usage, node, input_name, str(value)))


def switch(one, usage, node, switch_name, entry):
    record("%s %s %s" % (one["name"], node, switch_name),
           builder().set_static_switch(system_path(), one["name"], usage, node, switch_name, entry))


def nest(one, usage, node, input_name, script):
    return record("%s %s %s" % (one["name"], node, input_name), str(
        builder().set_input_dynamic_input(system_path(), one["name"], usage, node, input_name, script)))


def link(one, usage, node, input_name, parameter):
    record("%s %s %s" % (one["name"], node, input_name),
           builder().set_input_linked_parameter(system_path(), one["name"], usage, node, input_name, parameter))


def nodes(one):
    return NODES[one["name"]]


def add_modules(one):
    """Fades go below the colour they scale; everything else appends."""
    cdo = builder()
    system = system_path()
    name = one["name"]
    NODES[name] = {}
    if one["kind"] == "beam":
        # The template already feeds its beam start, and a dynamic input only attaches where the stack holds a
        # plain value, so its own setup is disabled and a second one built on instead.
        record("%s disable template beam" % name,
               cdo.set_module_enabled(system, name, EMITTER_UPDATE, BEAM_SETUP, False))
        NODES[name]["beam"] = str(cdo.add_module(system, name, EMITTER_UPDATE, MODULE_BEAM_SETUP))
        NODES[name]["hold"] = BEAM_COLOR
        NODES[name]["taper"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_RIBBON_WIDTH))
        NODES[name]["fade"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_COLOR))
        switch(one, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Once")
        switch(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime Mode", "Direct Set")
        return

    record("%s disable template fade" % name,
           cdo.set_module_enabled(system, name, PARTICLE_UPDATE, TEMPLATE_FADE, False))
    if one["rate"]:
        record("%s disable template burst" % name,
               cdo.set_module_enabled(system, name, EMITTER_UPDATE, BURST, False))
        NODES[name]["rate"] = str(cdo.add_module(system, name, EMITTER_UPDATE, MODULE_SPAWN_RATE))
    NODES[name]["place"] = str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_SHAPE_LOCATION))
    if one["shot"]:
        NODES[name]["shot"] = str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_VELOCITY))
    if one["burst_out"]:
        NODES[name]["out"] = str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_VELOCITY_FROM_POINT))
    if one["spin"]:
        NODES[name]["spin"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SPRITE_ROTATION_RATE))
    NODES[name]["hold"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_COLOR))
    if one["grow"] or one["shrink"]:
        NODES[name]["size"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_SPRITE_SIZE))
    if one["fade"]:
        NODES[name]["fade"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_COLOR))

    switch(one, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Once")
    for switch_name in ("Lifetime Mode", "Color Mode"):
        switch(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch_name, "Direct Set")
    # Offset Mode's other entry, None, hides the Offset input the lift is written to.
    for switch_name, entry in (("Shape Primitive", "Ring / Disc"), ("Ring / Disc Mode", "Circle"),
                               ("Offset Mode", "Default")):
        switch(one, PARTICLE_SPAWN, nodes(one)["place"], switch_name, entry)
    if one["lattice"]:
        switch(one, PARTICLE_SPAWN, nodes(one)["place"], "Ring / Disc Distribution Mode", "Uniform")
    if "size" in nodes(one):
        switch(one, PARTICLE_UPDATE, nodes(one)["size"], "Scale Sprite Size Mode", "Uniform")


def nest_first(one):
    """Every input fed from something computed, one level down."""
    nodes(one)["retint"] = nest(one, PARTICLE_UPDATE, nodes(one)["hold"], "Color", DYN_MULTIPLY_COLOR)
    if one["kind"] == "beam":
        nodes(one)["start"] = nest(one, EMITTER_UPDATE, nodes(one)["beam"], "Beam Start",
                                   DYN_ADD_VECTOR_TO_POSITION)
        nodes(one)["end"] = nest(one, EMITTER_UPDATE, nodes(one)["beam"], "Beam End",
                                 DYN_CONVERT_VECTOR_TO_POSITION)
        nodes(one)["narrow"] = nest(one, PARTICLE_UPDATE, nodes(one)["taper"], "Ribbon Width Scale",
                                    DYN_ONE_MINUS_FLOAT)
        nodes(one)["alpha"] = nest(one, PARTICLE_UPDATE, nodes(one)["fade"], "Scale Alpha", DYN_ONE_MINUS_FLOAT)
        return

    nodes(one)["tint"] = nest(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", DYN_MULTIPLY_COLOR)
    if one["along"]:
        nodes(one)["offset"] = nest(one, PARTICLE_SPAWN, nodes(one)["place"], "Offset", DYN_MAKE_VECTOR)
    if one["stagger"]:
        nodes(one)["life"] = nest(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", DYN_SCALE_AND_BIAS)
    if one["width_sized"]:
        nodes(one)["width"] = nest(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size",
                                   DYN_MULTIPLY_FLOAT)
    if one["shot"]:
        nodes(one)["velocity"] = nest(one, PARTICLE_SPAWN, nodes(one)["shot"], "Velocity", DYN_MAKE_VECTOR)
    if one["shrink"]:
        nodes(one)["scale"] = nest(one, PARTICLE_UPDATE, nodes(one)["size"], "Uniform Scale Factor",
                                   DYN_ONE_MINUS_FLOAT)
    if one["fade"]:
        nodes(one)["alpha"] = nest(one, PARTICLE_UPDATE, nodes(one)["fade"], "Scale Alpha", DYN_ONE_MINUS_FLOAT)


def nest_second(one):
    """The endpoints' vectors, and the ray's length read off the lattice index and the shot's speed."""
    if one["kind"] == "beam":
        record("%s Beam Start anchor" % one["name"], builder().set_input_dynamic_input(
            system_path(), one["name"], EMITTER_UPDATE, nodes(one)["start"], "Position", DYN_SIMULATION_POSITION))
        nodes(one)["start lift"] = nest(one, EMITTER_UPDATE, nodes(one)["start"], "Vector", DYN_MAKE_VECTOR)
        nodes(one)["reach"] = nest(one, EMITTER_UPDATE, nodes(one)["end"], "Input Position", DYN_MAKE_VECTOR)
        return

    if one["along"]:
        nodes(one)["along"] = nest(one, PARTICLE_SPAWN, nodes(one)["offset"], "X", DYN_MULTIPLY_FLOAT)
    if one["stagger"]:
        nodes(one)["life index"] = nest(one, PARTICLE_SPAWN, nodes(one)["life"], "Float", DYN_NORMALIZED_INDEX)
    if one["shot"]:
        nodes(one)["speed"] = nest(one, PARTICLE_SPAWN, nodes(one)["velocity"], "X", DYN_MULTIPLY_FLOAT)


def nest_third(one):
    if one["kind"] == "shapes" and one["along"]:
        nodes(one)["along index"] = nest(one, PARTICLE_SPAWN, nodes(one)["along"], "B", DYN_NORMALIZED_INDEX)


def set_values(one):
    for usage, key in ((PARTICLE_SPAWN, "tint"), (PARTICLE_UPDATE, "retint")):
        if key in nodes(one):
            link(one, usage, nodes(one)[key], "Linear Color", "User.Color")
            write(one, usage, nodes(one)[key], "Float", one["brightness"])
    if "alpha" in nodes(one):
        link(one, PARTICLE_UPDATE, nodes(one)["alpha"], "Float", "Particles.NormalizedAge")

    if one["kind"] == "beam":
        write(one, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", one["life"])
        for input_name, value in (("Spawn Count", one["points"]), ("Spawn Probability", 1.0),
                                  ("Spawn Time", 0), ("Loop Count Limit", 0)):
            write(one, EMITTER_UPDATE, BURST, input_name, value)
        write(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", one["life"])
        write(one, PARTICLE_SPAWN, BEAM_WIDTH_CURVE, "Scale Curve", one["width"])
        for axis, value in (("X", 0), ("Y", 0), ("Z", LIFT)):
            write(one, EMITTER_UPDATE, nodes(one)["start lift"], axis, value)
        link(one, EMITTER_UPDATE, nodes(one)["reach"], "X", "User.Beam_Length")
        write(one, EMITTER_UPDATE, nodes(one)["reach"], "Y", 0)
        write(one, EMITTER_UPDATE, nodes(one)["reach"], "Z", LIFT)
        link(one, PARTICLE_UPDATE, nodes(one)["narrow"], "Float", "Particles.NormalizedAge")
        return

    write(one, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", one["window"] or one["life"] + one["stagger"])
    if one["rate"]:
        write(one, EMITTER_UPDATE, nodes(one)["rate"], "SpawnRate", one["rate"])
    else:
        for input_name, value in (("Spawn Count", one["count"]), ("Spawn Probability", 1.0),
                                  ("Spawn Time", 0), ("Loop Count Limit", 0)):
            write(one, EMITTER_UPDATE, BURST, input_name, value)

    if one["stagger"]:
        write(one, PARTICLE_SPAWN, nodes(one)["life"], "Scale", one["stagger"])
        write(one, PARTICLE_SPAWN, nodes(one)["life"], "Bias", one["life"])
    else:
        write(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", one["life"])
    if one["width_sized"]:
        link(one, PARTICLE_SPAWN, nodes(one)["width"], "A", "User.Beam_Width")
        write(one, PARTICLE_SPAWN, nodes(one)["width"], "B", one["size"])
    else:
        write(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", one["size"])

    write(one, PARTICLE_SPAWN, nodes(one)["place"], "Ring Radius", one["radius"] * HOLE_RADIUS)
    if one["along"]:
        link(one, PARTICLE_SPAWN, nodes(one)["along"], "A", "User.Beam_Length")
        write(one, PARTICLE_SPAWN, nodes(one)["offset"], "Y", 0)
        write(one, PARTICLE_SPAWN, nodes(one)["offset"], "Z", LIFT)
    else:
        write(one, PARTICLE_SPAWN, nodes(one)["place"], "Offset", "0,0,%s" % LIFT)
    if not one["lattice"]:
        write(one, PARTICLE_SPAWN, nodes(one)["place"], "Disc Coverage", 1.0)

    if one["shot"]:
        # The speed that reaches the ray's end on the particle's last frame.
        link(one, PARTICLE_SPAWN, nodes(one)["speed"], "A", "User.Beam_Length")
        write(one, PARTICLE_SPAWN, nodes(one)["speed"], "B", one["shot"] / one["life"])
        write(one, PARTICLE_SPAWN, nodes(one)["velocity"], "Y", 0)
        write(one, PARTICLE_SPAWN, nodes(one)["velocity"], "Z", 0)
    if one["burst_out"]:
        # Pushed away from a centre at the lift, so the push stays flat.
        write(one, PARTICLE_SPAWN, nodes(one)["out"], "Velocity Strength", one["burst_out"])
        write(one, PARTICLE_SPAWN, nodes(one)["out"], "Origin Offset", "0,0,%s" % LIFT)
    if one["spin"]:
        write(one, PARTICLE_UPDATE, nodes(one)["spin"], "Rotation Rate", one["spin"])
    if one["grow"]:
        link(one, PARTICLE_UPDATE, nodes(one)["size"], "Uniform Scale Factor", "Particles.NormalizedAge")
    if one["shrink"]:
        link(one, PARTICLE_UPDATE, nodes(one)["scale"], "Float", "Particles.NormalizedAge")


def main():
    """Each phase runs before the compile that makes the nodes it added addressable."""
    cdo = builder()
    system = system_path()
    for one in LAYERS:
        record("emitter %s" % one["name"], cdo.add_emitter(
            system, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, one["name"], one["name"])))
    for parameter, value in (("User.Color", DEFAULT_COLOR), ("User.Beam_Length", DEFAULT_LENGTH),
                             ("User.Beam_Width", DEFAULT_HALF_WIDTH)):
        record(parameter, cdo.set_user_parameter(system, parameter, str(value)))
    record("compile emitters", cdo.compile_and_save(system))

    for phase in (add_modules, nest_first, nest_second, nest_third):
        for one in LAYERS:
            phase(one)
        record("compile %s" % phase.__name__, cdo.compile_and_save(system))

    for one in LAYERS:
        set_values(one)
    record("compile", cdo.compile_and_save(system))
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_SacrificeRelease.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(prepare())
