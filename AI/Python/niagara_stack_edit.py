"""
Niagara emitter stack editing via GeoNiagaraBuilderUtil.
Reference: AI/MCP/MCP_Niagara.md

Run through MCP execute_script. A function added to a stage is only addressable by value after a compile,
so add_module_with_switches and set_input_values are deliberately separate passes.

Nothing printed here comes back through the tool; dump_stage logs to LogTemp, read it with get_output_log.
"""

import unreal


def builder():
    return unreal.GeoNiagaraBuilderUtil.get_default_object()


def duplicate_system(source_path, folder, new_name):
    """NiagaraSystem assets only duplicate through AssetTools; EditorAssetLibrary returns None for them."""
    source = unreal.load_object(None, source_path)
    duplicate = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(new_name, folder, source)
    if duplicate:
        unreal.EditorAssetLibrary.save_asset(duplicate.get_path_name())
    return duplicate


def dump_stage(system_path, emitter_name, usage):
    """Logs each function node of one stage with its switches and its value inputs' types."""
    builder().list_stack(system_path, emitter_name, usage)


def add_module_with_switches(system_path, emitter_name, usage, module_script_path, switches=None,
                             target_index=-1):
    """Adds a module, applies its static switches, compiles so its value inputs exist, returns its name."""
    cdo = builder()
    function_name = cdo.add_module(system_path, emitter_name, usage, module_script_path, target_index)
    if not function_name or str(function_name) == "None":
        return None
    for switch_name, entry_display_name in (switches or {}).items():
        cdo.set_static_switch(system_path, emitter_name, usage, function_name, switch_name, entry_display_name)
    cdo.compile_and_save(system_path)
    return function_name


def set_input_values(system_path, emitter_name, usage, function_name, values):
    """Writes constants on an already-compiled function; each value is a comma separated component list."""
    cdo = builder()
    for input_name, value in values.items():
        cdo.set_input_value(system_path, emitter_name, usage, function_name, input_name, value)
    cdo.compile_and_save(system_path)


def nest_dynamic_input(system_path, emitter_name, usage, function_name, input_name, dynamic_input_path):
    """Feeds one input from a dynamic input; the returned name is itself a valid target for these calls."""
    return builder().set_input_dynamic_input(system_path, emitter_name, usage, function_name, input_name,
                                             dynamic_input_path)


def set_module_enabled(system_path, emitter_name, usage, function_name, enabled):
    """Disabling replaces removal — remove-module-from-stack is not linkable."""
    return builder().set_module_enabled(system_path, emitter_name, usage, function_name, enabled)


SHAPE_LOCATION = "/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation"

def example_reshape_debris_burst_to_cone():
    """Reshapes a grid-spawned debris burst into a cone, leaving the particle count module untouched.

    Deliberately not invoked — adding a module is not idempotent, so a second run appends a second one.
    A spherical cone's widest radius is Length * sin(Angle / 2), with Angle the full aperture in degrees.
    """
    system = "/Game/Art/VFX/Assets/NS_DeathDebrisTriangle.NS_DeathDebrisTriangle"
    emitter = "UpwardMeshBurst"
    spawn = unreal.NiagaraScriptUsage.PARTICLE_SPAWN_SCRIPT

    shape = add_module_with_switches(system, emitter, spawn, SHAPE_LOCATION, {"Shape Primitive": "Cone"})
    if shape:
        set_module_enabled(system, emitter, spawn, "GridLocation", False)
        set_input_values(system, emitter, spawn, shape, {
            "Cone Angle": "60",
            "Cone Length": "80",
            "Cone Inner Angle": "0",
            "Cone Surface Distribution": "0",
        })
    dump_stage(system, emitter, spawn)
