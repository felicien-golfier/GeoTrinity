"""The five auras a character wears, rebuilt as geometry.

Every system UGameDataSettings::BuffVFX plays on a BODY, into /Game/Art/VFX/Generic/Niagara. The two worn
by a SHOT keep their own builders and are not touched here — a shot lends its flight path to a ribbon, and a
path is already a shape.

  system            attribute               reads as
  NS_ChargedHalo    DamageMultiplier        six solid triangles orbiting inside one hollow one, snapping
  NS_VitalHalo      AppliedHealBoost        two counter-turning dashed rings shedding motes
  NS_BulwarkShell   DamageReduction         two hexagon shells, one dashed, plates riding between them
  NS_MendingDrift   ReceivedHealBoost       hollow triangles drawn inward and shrinking into the body
  NS_SwiftWake      MovementSpeedMultiplier chevrons dropped along the path and left behind

Every one of them sheds. The small fast layer of each aura simulates in WORLD space, so its shapes are left
where they were born and the body walks out of them: standing still that layer is a pulse, running it is a
trail, and it is the only part of an aura that can show speed at all — everything held rides along with the
owner and therefore cannot. A shed shape also drifts slowly out of the body, both to keep the idle pulse
alive and because a strand has to be drawn along something.

A body standing still lends no path, so an aura's shape has to be in the elements and in where they sit.
Both come from one place: every layer draws M_GeoShape instances — a polygon, hollow or solid, crisp at any
size — and places them on exact angular divisions rather than scattering them. See AI/ArtDirection.md.

The lattice is the whole trick. A burst of N particles carries execution indices 0..N-1, and the normalized
index fed into the ring's U position spaces them at exactly 360/N with no randomness anywhere. Held for the
system's whole life and turned as one by a vortex, that ring is rigid: it rotates without ever breathing or
clumping, which is what separates a machine from a cloud. Rate-spawned layers have no index to divide, so
they take the ring at random — allowed only where something else already carries the structure, which for
the wake is the path and for the drift is the inward pull.

Rotation is the other half. A sprite spinning at a constant rate reads as manufactured, and it is the only
way a ring shows that it turns at all: a circle rotating is a circle, which is why the ring instances are
dashed. Shapes never blur out — they shrink and fade together, keeping their edges to the last frame.

Sizes are absolute, in unreal units, because they are what the eye reads; ring radii are fractions of
`User.Radius`, which is the body they are fitted to. Nothing writes that at runtime — a buff system is
spawned with no parameters — so it is authored to the playable capsule.

A layer shed at the body can also carry a strand: a ribbon renderer beside its sprites, threading the shapes
in birth order, which along a path is the path. One ribbon chains several particles, so no single shape
trails itself, and a layer spawned around a ring zigzags between its own shapes rather than drawing one.

A run is `prepare()`, then the `niagara_ops` route to drop each base system's own emitter — the one step no
Python here reaches — then `main()`. The report names every builder call that returned false.

Reference: AI/ArtDirection.md, AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import math

import unreal

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
BASE_SYSTEM = "/Game/Art/VFX/Generic/Niagara/NS_CircleArround.NS_CircleArround"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_LayerSources"
TEMPLATE_SPRITE = "/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"
SHAPES = "/Game/Art/VFX/Generic/Materials/MatInstances"
RIBBON_CLASS = "/Script/Niagara.NiagaraRibbonRendererProperties"
RIBBON_MATERIAL = "/Niagara/DefaultAssets/DefaultRIbbonMaterial.DefaultRibbonMaterial"

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
MODULE_POINT_ATTRACTION = "/Niagara/Modules/Update/Forces/PointAttractionForce.PointAttractionForce"
MODULE_DRAG = "/Niagara/Modules/Update/Forces/Drag.Drag"
MODULE_SPRITE_ROTATION_RATE = "/Niagara/Modules/Update/Orientation/SpriteRotationRate.SpriteRotationRate"
MODULE_SCALE_SPRITE_SIZE = "/Niagara/Modules/Update/Size/ScaleSpriteSize.ScaleSpriteSize"
MODULE_SCALE_COLOR = "/Niagara/Modules/Update/Color/ScaleColor.ScaleColor"
MODULE_SCALE_RIBBON_WIDTH = "/Niagara/Modules/Ribbons/ScaleRibbonWidth.ScaleRibbonWidth"

DYN_MULTIPLY_FLOAT = "/Niagara/DynamicInputs/Multiply/Multiply_Float.Multiply_Float"
DYN_ONE_MINUS_FLOAT = "/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat"

BODY_RADIUS = 50.0  # the playable character's capsule radius
HELD = 100000.0  # a lifetime and a loop long enough that a burst never fires twice
CURVE_TENSION = 0.99  # tension IS sharpness: a slack strand rounds its corners off and reads as a noodle


def layer(name, shape, color, size, radius=0.0, count=1, rate=0.0, life=HELD, loop=HELD,
          lattice=False, spin=0.0, orbit=0.0, pull=(), drift=0.0, fade=False, shrink=False, follow=True,
          ribbon=0.0):
    """One emitter. `count` bursts on a lattice, `rate` spawns over time and cannot be spaced.
    `follow` rides with the owner; clearing it leaves the shapes in the world, where the body abandons them.
    `ribbon` is a strand width threading the layer's own shapes, and needs them born at the body: one
    chains several particles in birth order, so a layer spawned around a ring zigzags between them."""
    return {"name": name, "shape": "%s/%s.%s" % (SHAPES, shape, shape), "color": color, "size": size,
            "radius": radius, "count": count, "rate": rate, "life": life, "loop": loop,
            "lattice": lattice, "spin": spin, "orbit": orbit, "pull": pull, "drift": drift, "fade": fade,
            "shrink": shrink, "follow": follow, "ribbon": ribbon}


SYSTEMS = (
    # Damage boost: blades pointing out of a frame that turns the other way, over a snapping beat.
    ("NS_ChargedHalo", (
        layer("Frame", "MI_GeoShape_TriThin", "1.20,0.10,3.00,1", 175.0, spin=18.0),
        layer("Blades", "MI_GeoShape_TriSolid", "3.00,0.20,7.00,1", 30.0,
              radius=1.60, count=6, lattice=True, orbit=70.0, spin=-140.0),
        layer("Snap", "MI_GeoShape_TriSolid", "6.00,2.00,9.00,1", 22.0,
              radius=2.30, count=3, lattice=True, life=0.22, loop=0.55, spin=260.0,
              fade=True, shrink=True),
        # On the blades' own ring and far under their size, so it reads as what the blades leave behind
        # rather than as a second set of triangles.
        layer("Shed", "MI_GeoShape_TriSolid", "2.40,0.15,5.60,1", 8.0,
              radius=1.60, rate=10.0, life=0.55, spin=210.0, drift=45.0,
              fade=True, shrink=True, follow=False),
    )),
    # Applied heal: two dashed rings on unrelated periods, and motes shed off the outer one.
    ("NS_VitalHalo", (
        layer("RingOuter", "MI_GeoShape_RingDash", "1.60,4.20,0.30,1", 165.0, spin=34.0),
        layer("RingInner", "MI_GeoShape_RingDash4", "2.60,6.50,0.40,1", 120.0, spin=-46.0),
        layer("Motes", "MI_GeoShape_Ring", "3.20,8.00,0.60,1", 18.0,
              radius=1.55, rate=9.0, life=0.95, spin=90.0, drift=30.0,
              fade=True, shrink=True, follow=False),
    )),
    # Damage reduction: a closed shell, tight to the body, with plates riding between its two skins.
    ("NS_BulwarkShell", (
        layer("ShellOuter", "MI_GeoShape_Hex", "0.30,0.85,5.50,1", 180.0, spin=14.0),
        layer("ShellInner", "MI_GeoShape_HexDash", "0.50,1.40,7.50,1", 132.0, spin=-22.0),
        layer("Plates", "MI_GeoShape_QuadSolid", "0.60,1.80,9.00,1", 22.0,
              radius=1.45, count=6, lattice=True, orbit=18.0, spin=30.0),
        layer("Shards", "MI_GeoShape_QuadSolid", "0.45,1.30,6.50,1", 9.0,
              radius=1.45, rate=10.0, life=0.60, spin=120.0, drift=32.0,
              fade=True, shrink=True, follow=False),
    )),
    # Received heal: the shapes come to the body and shrink into it, which is the whole read.
    ("NS_MendingDrift", (
        layer("Draw", "MI_GeoShape_Tri", "0.60,5.50,1.20,1", 22.0,
              radius=3.40, rate=8.0, life=1.00, spin=120.0, pull=(280.0, 400.0, 2.2),
              fade=True, shrink=True),
        # Left in the world, its pull still resolves to wherever the body now is, so a running body drags
        # a stream of sparks after it and a standing one gathers them straight in.
        layer("Spark", "MI_GeoShape_TriSolid", "1.80,7.50,2.40,1", 14.0,
              radius=2.60, rate=7.0, life=0.70, spin=-200.0, pull=(420.0, 400.0, 2.6),
              fade=True, shrink=True, follow=False),
    )),
    # Movement speed: dropped on the path and left there, so it draws only while the body runs.
    ("NS_SwiftWake", (
        # Born at the body and left there, so birth order is the path itself and the strand threads the
        # chevrons along it — drawn by the ground covered, gone the moment the body stops.
        layer("Chevron", "MI_GeoShape_Tri", "5.00,2.60,0.25,1", 28.0,
              rate=16.0, life=0.45, spin=60.0, fade=True, shrink=True, follow=False, ribbon=10.0),
        layer("Dust", "MI_GeoShape_TriSolid", "8.00,4.20,0.40,1", 13.0,
              radius=0.50, rate=22.0, life=0.30, spin=-300.0, drift=40.0,
              fade=True, shrink=True, follow=False),
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


def shape_material(source, one):
    """Swapping this material instance is what swaps the shape — hollow, solid, dashed, any polygon.

    Set on the layer SOURCE, before the emitter is copied into a system: inside a system an emitter's
    object name is not its handle name, so a renderer looked up by name there can belong to another copy.
    """
    sprite = unreal.find_object(source, "NiagaraSpriteRendererProperties_0")
    if not sprite:
        record("%s renderer" % one["name"], "NOT FOUND")
        return
    sprite.set_editor_property("Material", unreal.load_object(None, one["shape"]))
    record("%s renderer" % one["name"], one["shape"].split(".")[-1])


def strand_renderer(source, one):
    """A second renderer beside the sprites, drawing one strand through the shapes the layer already sheds.

    Python reaches no ribbon class, so the shim creates it; every property on it is then plain Python.
    """
    if not one["ribbon"]:
        return
    name = builder().add_renderer(source.get_path_name(), RIBBON_CLASS)
    ribbon = unreal.find_object(source, str(name))
    if not ribbon:
        record("%s strand" % one["name"], "NOT FOUND")
        return
    ribbon.set_editor_property("Material", unreal.load_object(None, RIBBON_MATERIAL))
    ribbon.set_editor_property("CurveTension", CURVE_TENSION)
    record("%s strand" % one["name"], name)


def write_local_space(source, one):
    """Rides the owner, or is left in the world where the body abandons it.

    Lives in the emitter's versioned data, which both Python bindings refuse — the shim reaches it, and only
    on an emitter asset, so like the material it is written before the emitter is copied into a system.
    """
    record("%s local space" % one["name"], builder().set_emitter_property(
        source.get_path_name(), "bLocalSpace", "true" if one["follow"] else "false"))


def prepare():
    """Rewrites every target and every layer source; dropping each base's own emitter goes next."""
    for system_name, layers in SYSTEMS:
        duplicate_into(BASE_SYSTEM, TARGET_FOLDER, system_name)
        for one in layers:
            source = duplicate_into(TEMPLATE_SPRITE, LAYER_SOURCE_FOLDER, one["name"])
            if source:
                shape_material(source, one)
                strand_renderer(source, one)
                write_local_space(source, one)
                unreal.EditorAssetLibrary.save_loaded_asset(source)
    return "\n".join(LOG)


def write(system, one, usage, node, input_name, value):
    record("%s %s %s" % (one["name"], node, input_name),
           builder().set_input_value(system, one["name"], usage, node, input_name, str(value)))


def switch(system, one, usage, node, switch_name, entry):
    record("%s %s %s" % (one["name"], node, switch_name),
           builder().set_static_switch(system, one["name"], usage, node, switch_name, entry))


def nest(system, one, usage, node, input_name, script):
    return record("%s %s %s" % (one["name"], node, input_name), str(
        builder().set_input_dynamic_input(system, one["name"], usage, node, input_name, script)))


def link(system, one, usage, node, input_name, parameter):
    record("%s %s %s" % (one["name"], node, input_name),
           builder().set_input_linked_parameter(system, one["name"], usage, node, input_name, parameter))


def nodes(one):
    return NODES[one["name"]]


def add_modules(system, one):
    """A rate reads as a state and a burst as an event, so only one of the two is ever left enabled.
    Forces go above the solver that integrates them; everything else appends."""
    cdo = builder()
    name = one["name"]
    NODES[name] = {}
    record("%s disable template fade" % name,
           cdo.set_module_enabled(system, name, PARTICLE_UPDATE, TEMPLATE_FADE, False))
    if one["rate"]:
        record("%s disable template burst" % name,
               cdo.set_module_enabled(system, name, EMITTER_UPDATE, BURST, False))
        NODES[name]["rate"] = str(cdo.add_module(system, name, EMITTER_UPDATE, MODULE_SPAWN_RATE))
    if one["radius"]:
        NODES[name]["place"] = str(cdo.add_module(system, name, PARTICLE_SPAWN, MODULE_SHAPE_LOCATION))
    if one["drift"]:
        # Below the placement it reads, and pushing from an origin the module resolves to the owner itself.
        NODES[name]["drift"] = str(cdo.add_module(system, name, PARTICLE_SPAWN,
                                                  MODULE_VELOCITY_FROM_POINT))
    if one["pull"]:
        NODES[name]["pull"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_POINT_ATTRACTION, 0))
        NODES[name]["drag"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_DRAG, 1))
    if one["orbit"]:
        NODES[name]["orbit"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_VORTEX_VELOCITY, 0))
    if one["spin"]:
        NODES[name]["spin"] = str(cdo.add_module(system, name, PARTICLE_UPDATE,
                                                 MODULE_SPRITE_ROTATION_RATE))
    if one["shrink"]:
        NODES[name]["shrink"] = str(cdo.add_module(system, name, PARTICLE_UPDATE,
                                                   MODULE_SCALE_SPRITE_SIZE))
    if one["fade"]:
        NODES[name]["fade"] = str(cdo.add_module(system, name, PARTICLE_UPDATE, MODULE_SCALE_COLOR))
    if one["ribbon"]:
        NODES[name]["width"] = str(cdo.add_module(system, name, PARTICLE_UPDATE,
                                                  MODULE_SCALE_RIBBON_WIDTH))

    switch(system, one, EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    for switch_name in ("Lifetime Mode", "Color Mode"):
        switch(system, one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, switch_name, "Direct Set")
    if one["ribbon"]:
        switch(system, one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Ribbon Width Mode", "Direct Set")
    if one["radius"]:
        for switch_name, entry in (("Shape Primitive", "Ring / Disc"), ("Ring / Disc Mode", "Circle")):
            switch(system, one, PARTICLE_SPAWN, nodes(one)["place"], switch_name, entry)
    if one["lattice"]:
        # The spacing is the module's own: Uniform lays a burst's execution indices around the ring at
        # exactly 360/N with no randomness and nothing to nest under it.
        switch(system, one, PARTICLE_SPAWN, nodes(one)["place"], "Ring / Disc Distribution Mode",
               "Uniform")
    if one["shrink"]:
        switch(system, one, PARTICLE_UPDATE, nodes(one)["shrink"], "Scale Sprite Size Mode", "Uniform")


def nest_inputs(system, one):
    if one["radius"]:
        nodes(one)["ring"] = nest(system, one, PARTICLE_SPAWN, nodes(one)["place"], "Ring Radius",
                                  DYN_MULTIPLY_FLOAT)
    if one["fade"]:
        nodes(one)["alpha"] = nest(system, one, PARTICLE_UPDATE, nodes(one)["fade"], "Scale Alpha",
                                   DYN_ONE_MINUS_FLOAT)
    if one["shrink"]:
        nodes(one)["scale"] = nest(system, one, PARTICLE_UPDATE, nodes(one)["shrink"],
                                   "Uniform Scale Factor", DYN_ONE_MINUS_FLOAT)
    if one["ribbon"]:
        nodes(one)["taper"] = nest(system, one, PARTICLE_UPDATE, nodes(one)["width"],
                                   "Ribbon Width Scale", DYN_ONE_MINUS_FLOAT)


def set_values(system, one):
    write(system, one, EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", one["loop"])
    if one["rate"]:
        write(system, one, EMITTER_UPDATE, nodes(one)["rate"], "SpawnRate", one["rate"])
    else:
        for input_name, value in (("Spawn Count", one["count"]), ("Spawn Probability", 1.0),
                                  ("Spawn Time", 0), ("Loop Count Limit", 0)):
            write(system, one, EMITTER_UPDATE, BURST, input_name, value)

    write(system, one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", one["life"])
    write(system, one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", one["color"])
    write(system, one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", one["size"])
    if one["ribbon"]:
        write(system, one, PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Ribbon Width", one["ribbon"])

    if one["radius"]:
        link(system, one, PARTICLE_SPAWN, nodes(one)["ring"], "A", "User.Radius")
        write(system, one, PARTICLE_SPAWN, nodes(one)["ring"], "B", one["radius"])
        # Uniform is its own branch and exposes neither of these; the ring it lays is already the whole edge.
        if not one["lattice"]:
            write(system, one, PARTICLE_SPAWN, nodes(one)["place"], "Disc Coverage", 0.0)

    if one["drift"]:
        write(system, one, PARTICLE_SPAWN, nodes(one)["drift"], "Velocity Strength", one["drift"])
    if one["pull"]:
        strength, reach, drag = one["pull"]
        write(system, one, PARTICLE_UPDATE, nodes(one)["pull"], "AttractionStrength", strength)
        write(system, one, PARTICLE_UPDATE, nodes(one)["pull"], "Attraction Radius", reach)
        write(system, one, PARTICLE_UPDATE, nodes(one)["drag"], "Drag", drag)
    if one["orbit"]:
        # A vortex sets a linear speed and every particle rides the same circle, so the angular rate the
        # ring is authored in becomes that speed once.
        radius = one["radius"] * BODY_RADIUS
        write(system, one, PARTICLE_UPDATE, nodes(one)["orbit"], "Velocity Amount",
              math.radians(one["orbit"]) * radius)
        write(system, one, PARTICLE_UPDATE, nodes(one)["orbit"], "Vortex Axis", "0,0,1")
        write(system, one, PARTICLE_UPDATE, nodes(one)["orbit"], "Influence Falloff Radius", radius * 4.0)
    if one["spin"]:
        write(system, one, PARTICLE_UPDATE, nodes(one)["spin"], "Rotation Rate", one["spin"])
    for key in ("alpha", "scale", "taper"):
        if key in nodes(one):
            link(system, one, PARTICLE_UPDATE, nodes(one)[key], "Float", "Particles.NormalizedAge")


def build(system_name, layers):
    """Each phase runs before the compile that makes the nodes it added addressable by value."""
    cdo = builder()
    system = system_path(system_name)
    for one in layers:
        record("%s emitter %s" % (system_name, one["name"]), cdo.add_emitter(
            system, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, one["name"], one["name"])))
    record("%s User.Radius" % system_name, cdo.set_user_parameter(system, "User.Radius",
                                                                  str(BODY_RADIUS)))
    record("%s compile emitters" % system_name, cdo.compile_and_save(system))

    for phase in (add_modules, nest_inputs):
        for one in layers:
            phase(system, one)
        record("%s compile %s" % (system_name, phase.__name__), cdo.compile_and_save(system))

    for one in layers:
        set_values(system, one)
    record("%s compile" % system_name, cdo.compile_and_save(system))


def main():
    for system_name, layers in SYSTEMS:
        build(system_name, layers)
    unreal.EditorAssetLibrary.delete_directory(LAYER_SOURCE_FOLDER)
    return "\n".join(LOG)


if __name__ == "__main__":
    report = "%sGeoTrinity_GeoAura.txt" % unreal.Paths.project_saved_dir()
    open(report, "w").write(main())
