"""The energy filling the Square badge's keyhole while the sacrifice beam channels.

Builds /Game/Art/VFX/Assets/NS_Square_SacrificeHole, made to be attached to the SKM_SquareBadge `SacrificeHole`
socket. Every layer simulates in local space, so the whole effect rides the socket and follows the body's
gulps.

  layer        reads as
  Heart        one solid square at the centre, turning slowly
  Churn        four solid squares orbiting fast inside the hole
  Cage         four hollow squares counter-orbiting at the hole's rim
  Crackle      tiny solid squares flashing anywhere in the hole
  Wave         a thin square frame, wider than the body, collapsing into the centre while it turns
  Inflow       eight solid squares on a ring outside the body, travelling into the centre

Heart, Churn, Cage and Crackle are the energy held in the hole; Wave and Inflow are the suction, alternating
on one beat so something is always being drawn in.

The socket sits at the mesh's mid height, inside the block, so every layer is lifted above the block's top
face: whatever is not drawn inside the hole's own outline would otherwise be hidden by the body.

Colour is `User.Color`, the beam's palette colour pushed by UGeoBeamVFXComponent; each layer multiplies it by
its own brightness, which is what separates core from halo. `User.Radius` is the keyhole radius, and the
lattice radii are fractions of it.

A run is `prepare()`, then the `niagara_ops` route to drop the base system's own emitter (`Explosion`) —
the one step no Python here reaches — then `main()`. The report names every builder call that returned false.

Reference: AI/ArtDirection.md, AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import math

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Assets"
SYSTEM_NAME = "NS_Square_SacrificeHole"
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_Round_ZoneIndicator.NS_Round_ZoneIndicator"  # carries User.Color
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Assets/_SacrificeHoleSources"
TEMPLATE_SPRITE = "/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"
SHAPES = "/Game/Art/VFX/Generic/Materials/MatInstances"
SHAPE_PARENT = "/Game/Art/VFX/Generic/Materials/M_GeoShape"
THIN_FRAME = "MI_GeoShape_QuadThin"
THIN_FRAME_PARAMETERS = {"Sides": 4.0, "Thickness": 0.07, "Fill": 0.0, "Feather": 0.012, "Glow": 0.10}

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"
TEMPLATE_FADE = "ScaleColor"  # the template's own, which holds alpha at ~0 at every age

MODULE_SPAWN_RATE = "/Niagara/Modules/Emitter/SpawnRate.SpawnRate"
MODULE_SHAPE_LOCATION = "/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"
MODULE_VELOCITY_FROM_POINT = "/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint.AddVelocityFromPoint"
MODULE_VORTEX_VELOCITY = "/Niagara/Modules/Update/Velocity/VortexVelocity.VortexVelocity"
MODULE_SPRITE_ROTATION_RATE = "/Niagara/Modules/Update/Orientation/SpriteRotationRate.SpriteRotationRate"
MODULE_SCALE_SPRITE_SIZE = "/Niagara/Modules/Update/Size/ScaleSpriteSize.ScaleSpriteSize"
MODULE_COLOR = "/Niagara/Modules/Update/Color/Color.Color"
MODULE_SCALE_COLOR = "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"

DYN_MULTIPLY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_Float.Multiply_Float"
DYN_ONE_MINUS_FLOAT = "/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat"
DYN_MULTIPLY_COLOR = "/Niagara/DynamicInputs/Multiply/Multiply_LinearColorByFloat.Multiply_LinearColorByFloat"

HOLE_RADIUS = 15.0  # SQ_HOLE's radius through the badge's image-to-world scale
LIFT = 55.0  # above the block's top face, measured from the socket at mid height
DEFAULT_COLOR = "0,0,1,0.7"  # the palette's DamageReduction, which the beam pushes
HELD = 100000.0  # a lifetime and a loop long enough that a burst never fires twice
BEAT = 0.6  # the suction's period: a wave, then an inflow half a beat later


def layer(name, shape, brightness, size, radius=0.0, count=1, rate=0.0, life=HELD, loop=HELD, offset=0.0,
          lattice=False, spin=0.0, orbit=0.0, inflow=False, shrink=False, fade_in=False):
    """One emitter. `count` bursts once per `loop`, `offset` seconds into it; `rate` spawns over time instead.
    `radius` is a fraction of the hole, and `inflow` travels it to the centre in exactly `life`."""
    return {"name": name, "shape": "%s/%s.%s" % (SHAPES, shape, shape), "brightness": brightness,
            "size": size, "radius": radius, "count": count, "rate": rate, "life": life, "loop": loop,
            "offset": offset, "lattice": lattice, "spin": spin, "orbit": orbit, "inflow": inflow,
            "shrink": shrink, "fade_in": fade_in}


LAYERS = (
    layer("Heart", "MI_GeoShape_QuadSolid", 12.0, 8.0, spin=90.0),
    layer("Churn", "MI_GeoShape_QuadSolid", 6.0, 4.5, radius=0.50, count=4, lattice=True,
          orbit=320.0, spin=-400.0),
    layer("Cage", "MI_GeoShape_Quad", 3.5, 6.5, radius=0.80, count=4, lattice=True,
          orbit=-210.0, spin=260.0),
    layer("Crackle", "MI_GeoShape_QuadSolid", 8.0, 2.5, radius=0.85, rate=24.0, life=0.14,
          spin=600.0, shrink=True),
    layer("Wave", THIN_FRAME, 3.0, 70.0, life=BEAT, loop=BEAT, spin=180.0, shrink=True, fade_in=True),
    layer("Inflow", "MI_GeoShape_QuadSolid", 5.0, 5.0, radius=3.6, count=8, life=BEAT * 0.75, loop=BEAT,
          offset=BEAT * 0.5, lattice=True, spin=360.0, inflow=True, shrink=True, fade_in=True),
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


def thin_frame():
    """A square outline thin enough to stay a line at the width of the whole body; rewritten in place."""
    path = "%s/%s" % (SHAPES, THIN_FRAME)
    instance = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            THIN_FRAME, SHAPES, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, unreal.load_asset(SHAPE_PARENT))
    for name, value in THIN_FRAME_PARAMETERS.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    record(THIN_FRAME, instance.get_path_name())


def prepare_source(one):
    """Material and local space go on the layer SOURCE, before it is copied into the system: inside a system
    an emitter's object name is not its handle name, so a lookup by name there can land on another copy."""
    source = duplicate_into(TEMPLATE_SPRITE, LAYER_SOURCE_FOLDER, one["name"])
    if not source:
        return
    sprite = unreal.find_object(source, "NiagaraSpriteRendererProperties_0")
    if sprite:
        sprite.set_editor_property("Material", unreal.load_object(None, one["shape"]))
    record("%s renderer" % one["name"], one["shape"].split(".")[-1] if sprite else "NOT FOUND")
    record("%s local space" % one["name"],
           builder().set_emitter_property(source.get_path_name(), "bLocalSpace", "true"))
    unreal.EditorAssetLibrary.save_loaded_asset(source)


def prepare():
    """Rewrites the target and every layer source; dropping the base's own emitter goes next."""
    thin_frame()
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
    """Every layer is placed by the shape module, whose offset is the lift; forces go above the solver."""
    cdo = builder()
    system = system_path()
    name = one["name"]
    NODES[name] = {}
    record("%s disable template fade" % name,
           cdo.set_module_enabled(system, name, PARTICLE_UPDATE, TEMPLATE_FADE, False))
    if one["rate"]:
        record("%s disable template burst" % name,
               cdo.set_module_enabled(system, name, EMITTER_UPDATE, BURST, False))
        NODES[name]["rate"] = str(cdo.add_module(system, name, EMITTER_UPDATE, MODULE_SPAWN_RATE))
    NODES[name]["place"] = str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_SHAPE_LOCATION))
    if one["inflow"]:
        NODES[name]["inflow"] = str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_VELOCITY_FROM_POINT))
    if one["orbit"]:
        NODES[name]["orbit"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_VORTEX_VELOCITY, 0))
    if one["spin"]:
        NODES[name]["spin"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SPRITE_ROTATION_RATE))
    # Re-tinted every frame so a held layer follows a colour changed after it spawned; above the fade it feeds.
    NODES[name]["hold"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_COLOR))
    if one["shrink"]:
        NODES[name]["shrink"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_SPRITE_SIZE))
    if one["fade_in"]:
        NODES[name]["fade"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_COLOR))

    switch(one, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    for switch_name in ("Lifetime Mode", "Color Mode"):
        switch(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch_name, "Direct Set")
    # Offset Mode's other entry, None, hides the Offset input the lift is written to.
    for switch_name, entry in (("Shape Primitive", "Ring / Disc"), ("Ring / Disc Mode", "Circle"),
                               ("Offset Mode", "Default")):
        switch(one, PARTICLE_SPAWN, nodes(one)["place"], switch_name, entry)
    if one["lattice"]:
        # Uniform lays a burst's execution indices around the ring at exactly 360/N, with no randomness.
        switch(one, PARTICLE_SPAWN, nodes(one)["place"], "Ring / Disc Distribution Mode", "Uniform")
    if one["shrink"]:
        switch(one, PARTICLE_UPDATE, nodes(one)["shrink"], "Scale Sprite Size Mode", "Uniform")


def nest_inputs(one):
    nodes(one)["tint"] = nest(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", DYN_MULTIPLY_COLOR)
    nodes(one)["retint"] = nest(one, PARTICLE_UPDATE, nodes(one)["hold"], "Color", DYN_MULTIPLY_COLOR)
    nodes(one)["ring"] = nest(one, PARTICLE_SPAWN, nodes(one)["place"], "Ring Radius", DYN_MULTIPLY_FLOAT)
    if one["shrink"]:
        nodes(one)["scale"] = nest(one, PARTICLE_UPDATE, nodes(one)["shrink"], "Uniform Scale Factor",
                                   DYN_ONE_MINUS_FLOAT)


def set_values(one):
    write(one, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", one["loop"])
    if one["rate"]:
        write(one, EMITTER_UPDATE, nodes(one)["rate"], "SpawnRate", one["rate"])
    else:
        for input_name, value in (("Spawn Count", one["count"]), ("Spawn Probability", 1.0),
                                  ("Spawn Time", one["offset"]), ("Loop Count Limit", 0)):
            write(one, EMITTER_UPDATE, BURST, input_name, value)

    write(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", one["life"])
    write(one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", one["size"])
    for usage, key in ((PARTICLE_SPAWN, "tint"), (PARTICLE_UPDATE, "retint")):
        link(one, usage, nodes(one)[key], "Linear Color", "User.Color")
        write(one, usage, nodes(one)[key], "Float", one["brightness"])

    link(one, PARTICLE_SPAWN, nodes(one)["ring"], "A", "User.Radius")
    write(one, PARTICLE_SPAWN, nodes(one)["ring"], "B", one["radius"])
    write(one, PARTICLE_SPAWN, nodes(one)["place"], "Offset", "0,0,%s" % LIFT)
    # Uniform is its own branch and exposes no coverage; a random ring fills the disc it bounds.
    if not one["lattice"]:
        write(one, PARTICLE_SPAWN, nodes(one)["place"], "Disc Coverage", 1.0)

    if one["inflow"]:
        # Pointed away from a centre at the lift, so the push stays flat; negative draws it in, and the speed
        # is the one that reaches the centre on the particle's last frame.
        write(one, PARTICLE_SPAWN, nodes(one)["inflow"], "Velocity Strength",
              -one["radius"] * HOLE_RADIUS / one["life"])
        write(one, PARTICLE_SPAWN, nodes(one)["inflow"], "Origin Offset", "0,0,%s" % LIFT)
    if one["orbit"]:
        # A vortex sets a linear speed, so the angular rate becomes that speed once at the ring's radius.
        radius = one["radius"] * HOLE_RADIUS
        write(one, PARTICLE_UPDATE, nodes(one)["orbit"], "Velocity Amount", math.radians(one["orbit"]) * radius)
        write(one, PARTICLE_UPDATE, nodes(one)["orbit"], "Vortex Axis", "0,0,1")
        write(one, PARTICLE_UPDATE, nodes(one)["orbit"], "Influence Falloff Radius", radius * 4.0)
    if one["spin"]:
        write(one, PARTICLE_UPDATE, nodes(one)["spin"], "Rotation Rate", one["spin"])
    if one["shrink"]:
        link(one, PARTICLE_UPDATE, nodes(one)["scale"], "Float", "Particles.NormalizedAge")
    if one["fade_in"]:
        link(one, PARTICLE_UPDATE, nodes(one)["fade"], "Scale Alpha", "Particles.NormalizedAge")


def main():
    """Each phase runs before the compile that makes the nodes it added addressable by value."""
    cdo = builder()
    system = system_path()
    for one in LAYERS:
        record("emitter %s" % one["name"], cdo.add_emitter(
            system, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, one["name"], one["name"])))
    record("User.Radius", cdo.set_user_parameter(system, "User.Radius", str(HOLE_RADIUS)))
    record("User.Color", cdo.set_user_parameter(system, "User.Color", DEFAULT_COLOR))
    record("compile emitters", cdo.compile_and_save(system))

    for phase in (add_modules, nest_inputs):
        for one in LAYERS:
            phase(one)
        record("compile %s" % phase.__name__, cdo.compile_and_save(system))

    for one in LAYERS:
        set_values(one)
    record("compile", cdo.compile_and_save(system))
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_SacrificeHole.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(prepare())
