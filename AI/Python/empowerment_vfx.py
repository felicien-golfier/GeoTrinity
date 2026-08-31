"""NS_Empower: a buff aura authored for a top-down orthographic view.

The camera looks straight down the Z axis, so vertical motion is invisible and a soft scatter reads as haze.
Everything here is a shape that moves in XY: a border made of the character's own silhouette, a ribbon
curling around it, and three countable points orbiting further out.

  Rim     sparks sampled off the character's mesh, drifting outward - the border is the mesh's own outline
  Wisp    a ribbon spawned on an offset point and swept by a vortex, so it trails a curling arc
  Shards  three orbiting points at a wider radius

Placement rules learned the hard way, all of which fail silently:
  - SkeletalMeshLocation needs Allow CPUAccess on the mesh's LOD 0, or the data interface returns the
    emitter position and the layer collapses into the character.
  - Initialize Particle's Position Offset is ignored in every Position Mode it offers, so an offset
    placement needs a real location module; ShapeLocation on Ring / Disc is the one used here.
  - RotateAroundPoint cannot be used at all: its graph fails with "The Lerp operation can only lerp between
    two positions", and the particle simply keeps its spawn position. Orbits come from VortexVelocity,
    which sets a tangential velocity every frame; the speed for a radius r and a period t is 2*pi*r/t.
  - The templates' ScaleColor holds alpha at ~0 at every age, so it stays disabled and the fade over life
    lives in the materials instead (they read Particle.RelativeTime).

Every layer carries InheritSourceMovement. A character aura wants local space, but bLocalSpace moved to the
versioned emitter data in 5.7 and Python only reaches the deprecated property, which is refused; without
some form of follow the whole effect simulates in world space and lags behind a moving character.

Run order: prepare(), structure(), then values() once the compile in structure() has made the inputs
addressable. Judge it with AI/Python/vfx_editor_preview.py, never in the asset editor preview, which has no
attach parent for the mesh sampling.

Reference: AI/MCP/MCP_Niagara.md, AI/VFX.md.
"""

import unreal
import traceback

TARGET_FOLDER = "/Game/Art/VFX/Generic/Niagara"
SYSTEM_NAME = "NS_Empower"
LAYER_SOURCE_FOLDER = "/Game/Art/VFX/Generic/Niagara/_EmpowerLayers"

TEMPLATE_SPRITE = "/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"
TEMPLATE_RIBBON = "/Niagara/DefaultAssets/Templates/Emitters/LocationBasedRibbon.LocationBasedRibbon"

EMITTER_UPDATE = unreal.NiagaraScriptUsage.EMITTER_UPDATE_SCRIPT
PARTICLE_SPAWN = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT
PARTICLE_UPDATE = unreal.NiagaraScriptUsage.PARTICLE_UPDATE_SCRIPT

M_SPAWN_RATE = "/Niagara/Modules/Emitter/SpawnRate.SpawnRate"
M_SKELETAL_MESH_LOCATION = "/Niagara/Modules/Spawn/Location/V2/SkeletalMeshLocation.SkeletalMeshLocation"
M_VELOCITY_FROM_POINT = "/Niagara/Modules/Spawn/Velocity/AddVelocityFromPoint.AddVelocityFromPoint"
M_SHAPE_LOCATION = "/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"
M_VORTEX_VELOCITY = "/Niagara/Modules/Update/Velocity/VortexVelocity.VortexVelocity"
M_DRAG = "/Niagara/Modules/Update/Forces/Drag.Drag"
M_INHERIT_MOVEMENT = "/Niagara/Modules/Update/Position/InheritSourceMovement.InheritSourceMovement"

TEMPLATES = {"Rim": TEMPLATE_SPRITE, "Wisp": TEMPLATE_RIBBON, "Shards": TEMPLATE_SPRITE}
LAYERS = tuple(TEMPLATES)

# Modules each layer needs, in stage order. Forces are inserted above the solver that reads them.
MODULES = {
    "Rim": (("rate", EMITTER_UPDATE, M_SPAWN_RATE, -1),
            ("mesh", PARTICLE_SPAWN, M_SKELETAL_MESH_LOCATION, -1),
            ("push", PARTICLE_SPAWN, M_VELOCITY_FROM_POINT, -1),
            ("drag", PARTICLE_UPDATE, M_DRAG, 0),
            ("follow", PARTICLE_UPDATE, M_INHERIT_MOVEMENT, -1)),
    "Wisp": (("rate", EMITTER_UPDATE, M_SPAWN_RATE, -1),
             ("ring", PARTICLE_SPAWN, M_SHAPE_LOCATION, -1),
             ("push", PARTICLE_SPAWN, M_VELOCITY_FROM_POINT, -1),
             ("vortex", PARTICLE_UPDATE, M_VORTEX_VELOCITY, 0),
             ("drag", PARTICLE_UPDATE, M_DRAG, 1),
             ("follow", PARTICLE_UPDATE, M_INHERIT_MOVEMENT, -1)),
    "Shards": (("ring", PARTICLE_SPAWN, M_SHAPE_LOCATION, -1),
               ("vortex", PARTICLE_UPDATE, M_VORTEX_VELOCITY, 0),
               ("follow", PARTICLE_UPDATE, M_INHERIT_MOVEMENT, -1)),
}

# add_module names each node after its script, suffixing a name the template already carries, so the names
# below are what structure() returns and values() can be run in a fresh process without it.
NODES = {
    "Rim": {"rate": "SpawnRate", "mesh": "SkeletalMeshLocation", "push": "AddVelocityFromPoint",
            "drag": "Drag", "follow": "InheritSourceMovement"},
    "Wisp": {"rate": "SpawnRate", "ring": "ShapeLocation", "push": "AddVelocityFromPoint",
             "vortex": "VortexVelocity", "drag": "Drag", "follow": "InheritSourceMovement"},
    "Shards": {"ring": "ShapeLocation", "vortex": "VortexVelocity",
               "follow": "InheritSourceMovement"},
}

DOT_MATERIAL = "/Game/Art/VFX/Generic/Materials/M_EmpowerDot.M_EmpowerDot"
WISP_MATERIAL = "/Game/Art/VFX/Generic/Materials/M_EmpowerWisp.M_EmpowerWisp"
MATERIALS = {"Rim": DOT_MATERIAL, "Wisp": WISP_MATERIAL, "Shards": DOT_MATERIAL}

CHARACTER_RADIUS = 50.0  # the playable character's capsule radius

EMITTER_STATE = "EmitterState"
BURST = "SpawnBurst_Instantaneous"
INITIALIZE_PARTICLE = "InitializeParticle"
TEMPLATE_FADE = "ScaleColor"

MESH_SWITCHES = (("Mesh Sampling Type", "Surface (Triangles)"),
                 ("Position Sampling", "Apply (Rigid)"),
                 ("Triangle Sampling Mode", "Random (All Triangles)"))

LOG = []


def builder():
    return unreal.GeoNiagaraBuilderUtil.get_default_object()


def record(label, value):
    LOG.append("%-54s %s" % (label, value))
    return value


def system_path():
    return "%s/%s.%s" % (TARGET_FOLDER, SYSTEM_NAME, SYSTEM_NAME)


def clear_asset(package_path):
    """Frees a package path; one the editor still holds is renamed aside instead."""
    if not unreal.EditorAssetLibrary.does_asset_exist(package_path):
        return True
    return (unreal.EditorAssetLibrary.delete_asset(package_path)
            or unreal.EditorAssetLibrary.rename_asset(package_path, package_path + "_Superseded"))


def prepare():
    """Creates an empty system and one emitter asset per layer, copied from its template."""
    clear_asset("%s/%s" % (TARGET_FOLDER, SYSTEM_NAME))
    system = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        SYSTEM_NAME, TARGET_FOLDER, unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    record("system", system.get_path_name() if system else "FAILED")
    unreal.EditorAssetLibrary.save_asset(system.get_path_name())
    for layer, template in TEMPLATES.items():
        clear_asset("%s/%s" % (LAYER_SOURCE_FOLDER, layer))
        copy = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
            layer, LAYER_SOURCE_FOLDER, unreal.load_object(None, template))
        record("emitter asset %s" % layer, copy.get_path_name() if copy else "FAILED")
        unreal.EditorAssetLibrary.save_asset(copy.get_path_name())
    return "\n".join(LOG)


def structure():
    """Adds every emitter and module, then compiles once so the inputs become addressable."""
    cdo = builder()
    system = system_path()
    for layer in LAYERS:
        record("emitter %s" % layer, cdo.add_emitter(
            system, "%s/%s.%s" % (LAYER_SOURCE_FOLDER, layer, layer)))
    for layer in LAYERS:
        added = {key: str(cdo.add_module(system, layer, usage, path, index))
                 for key, usage, path, index in MODULES[layer]}
        record("modules %s" % layer, added)
        if added != NODES[layer]:
            raise RuntimeError("node names moved for %s: %s" % (layer, added))
    record("compile", cdo.compile_and_save(system))
    return "\n".join(LOG)


def set_renderer_material(layer, material_path):
    """Renderer properties are plain UPROPERTYs; the sprite and ribbon renderers differ only in class."""
    emitter = unreal.find_object(unreal.load_object(None, system_path()), layer)
    for name in ("NiagaraSpriteRendererProperties_0", "NiagaraRibbonRendererProperties_0"):
        properties = unreal.find_object(emitter, name) if emitter else None
        if properties:
            properties.set_editor_property("material", unreal.load_object(None, material_path))
            return record("renderer %s" % layer, properties.get_editor_property("material").get_name())
    return record("renderer %s" % layer, "NOT FOUND")


def orbit_speed(world_radius, seconds_per_turn):
    return 2.0 * 3.14159265 * world_radius / seconds_per_turn


def values():
    cdo = builder()
    system = system_path()

    def switch(layer, usage, node, name, entry):
        return record("%s %s %s" % (layer, node, name),
                      cdo.set_static_switch(system, layer, usage, node, name, entry))

    def write(layer, usage, node, name, value):
        return record("%s %s %s = %s" % (layer, node, name, value),
                      cdo.set_input_value(system, layer, usage, node, name, str(value)))

    def enabled(layer, usage, node, state):
        return record("%s %s enabled %s" % (layer, node, state),
                      cdo.set_module_enabled(system, layer, usage, node, state))

    # Rim: the border. Sampled on the mesh and pushed gently outward, so the outline is the character's own
    # silhouette rather than a circle drawn around it.
    enabled("Rim", EMITTER_UPDATE, BURST, False)
    switch("Rim", EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    write("Rim", EMITTER_UPDATE, NODES["Rim"]["rate"], "SpawnRate", 140.0)
    for name, entry in MESH_SWITCHES:
        switch("Rim", PARTICLE_SPAWN, NODES["Rim"]["mesh"], name, entry)
    write("Rim", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", 0.75)
    write("Rim", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", "1.80,0.60,2.80,1")
    write("Rim", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", CHARACTER_RADIUS * 0.20)
    write("Rim", PARTICLE_SPAWN, NODES["Rim"]["push"], "Velocity Strength", 45.0)
    write("Rim", PARTICLE_UPDATE, NODES["Rim"]["drag"], "Drag", 3.5)
    enabled("Rim", PARTICLE_UPDATE, TEMPLATE_FADE, False)

    # Wisp: the moving part. Every particle spawns on the same point of a ring and the vortex sweeps it
    # around, so the ribbon behind them is a trail. Its lifetime is the arc length: live long enough to
    # come back round and the trail closes into the circle this effect is trying not to be.
    switch("Wisp", EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    write("Wisp", EMITTER_UPDATE, NODES["Wisp"]["rate"], "SpawnRate", 60.0)
    switch("Wisp", PARTICLE_SPAWN, NODES["Wisp"]["ring"], "Shape Primitive", "Ring / Disc")
    switch("Wisp", PARTICLE_SPAWN, NODES["Wisp"]["ring"], "Ring / Disc Distribution Mode", "Direct")
    write("Wisp", PARTICLE_SPAWN, NODES["Wisp"]["ring"], "Ring Radius", CHARACTER_RADIUS * 1.25)
    write("Wisp", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", 0.5)
    write("Wisp", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", "2.00,0.80,3.20,1")
    write("Wisp", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Ribbon Width", 14.0)
    write("Wisp", PARTICLE_SPAWN, NODES["Wisp"]["push"], "Velocity Strength", 70.0)
    write("Wisp", PARTICLE_UPDATE, NODES["Wisp"]["vortex"], "Vortex Axis", "0,0,1")
    write("Wisp", PARTICLE_UPDATE, NODES["Wisp"]["vortex"], "Velocity Amount",
          orbit_speed(CHARACTER_RADIUS * 1.25, 1.6))
    write("Wisp", PARTICLE_UPDATE, NODES["Wisp"]["vortex"], "Influence Falloff Radius", 400.0)
    write("Wisp", PARTICLE_UPDATE, NODES["Wisp"]["drag"], "Drag", 2.5)

    # Shards: three countable points further out, turning the other way.
    switch("Shards", EMITTER_UPDATE, EMITTER_STATE, "Loop Behavior", "Infinite")
    write("Shards", EMITTER_UPDATE, EMITTER_STATE, "Loop Duration", 4.0)
    write("Shards", EMITTER_UPDATE, BURST, "Spawn Count", 3)
    write("Shards", EMITTER_UPDATE, BURST, "Spawn Time", 0.0)
    write("Shards", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Lifetime", 4.0)
    write("Shards", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Color", "2.40,1.40,3.20,1")
    write("Shards", PARTICLE_SPAWN, INITIALIZE_PARTICLE, "Uniform Sprite Size", CHARACTER_RADIUS * 0.22)
    switch("Shards", PARTICLE_SPAWN, NODES["Shards"]["ring"], "Shape Primitive", "Ring / Disc")
    switch("Shards", PARTICLE_SPAWN, NODES["Shards"]["ring"], "Ring / Disc Distribution Mode", "Random")
    write("Shards", PARTICLE_SPAWN, NODES["Shards"]["ring"], "Ring Radius", CHARACTER_RADIUS * 1.7)
    write("Shards", PARTICLE_UPDATE, NODES["Shards"]["vortex"], "Vortex Axis", "0,0,-1")
    write("Shards", PARTICLE_UPDATE, NODES["Shards"]["vortex"], "Velocity Amount",
          orbit_speed(CHARACTER_RADIUS * 1.7, 4.0))
    write("Shards", PARTICLE_UPDATE, NODES["Shards"]["vortex"], "Influence Falloff Radius", 400.0)
    enabled("Shards", PARTICLE_UPDATE, TEMPLATE_FADE, False)

    for layer in LAYERS:
        set_renderer_material(layer, MATERIALS[layer])
    record("compile", cdo.compile_and_save(system))
    return "\n".join(LOG)


def dump():
    """Logs every stage of every layer to LogTemp; read them out of the editor log."""
    cdo = builder()
    for layer in LAYERS:
        for usage in (EMITTER_UPDATE, PARTICLE_SPAWN, PARTICLE_UPDATE):
            unreal.log("GEODUMP ==== %s %s ====" % (layer, usage))
            cdo.list_stack(system_path(), layer, usage)
    return "\n".join(LOG)


def report_to(name, fn):
    path = "%sGeoTrinity_%s.txt" % (unreal.Paths.project_saved_dir(), name)
    try:
        open(path, "w").write(fn())
    except Exception:
        open(path, "w").write("\n".join(LOG) + "\n\nFAILED\n" + traceback.format_exc())
