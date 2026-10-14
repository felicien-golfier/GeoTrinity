"""Floor looks as material instances, the form an arena's Floors and the settings' BackgroundLooks take them in.

Each look is a child of the floor's own instance, so the lines stay tuned in one place, with one ML_Wave_* layer as
its glow. An existing look keeps its tuned values: only its glow layer is set again. Needs GeoTrinityEditor's
GeoMaterialBuilderUtil. Run through MCP execute_script, outside PIE.
"""
import unreal

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)


def layer_instance(folder, name, parent, layer_index, layer):
    """An instance of parent with layer at layer_index of its stack: created if missing, its values kept if not."""
    assert hasattr(unreal, "GeoMaterialBuilderUtil"), "GeoMaterialBuilderUtil missing: rebuild GeoTrinityEditor"
    instance = toolkit["load_or_create"](folder, name, unreal.MaterialInstanceConstant,
                                         unreal.MaterialInstanceConstantFactoryNew())
    if instance.get_editor_property("parent") != parent:
        unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)

    unreal.get_default_object(unreal.GeoMaterialBuilderUtil).set_instance_layer(instance, layer_index, layer)
    return instance


if __name__ == "__main__":
    BACKGROUND = "/Game/Art/VFX/Background"
    # The order Config/DefaultGame.ini lists them in BackgroundLooks, which is the order arenas cycle through.
    LOOKS = ("Spiral", "Radar", "Whirl", "Fireflies", "SoftRings", "ShockRings", "PolygonRings", "Halos",
             "Sierpinski", "Twinkle", "Rings")
    floor_instance = toolkit["load"](f"{BACKGROUND}/MI_BackgroundLattice")
    for look in LOOKS:
        layer_instance(f"{BACKGROUND}/Looks", f"MI_BackgroundLattice_{look}", floor_instance, 1,
                       toolkit["load"](f"{BACKGROUND}/Layers/ML_Wave_{look}"))

    unreal.log(f"BGLOOKS::instanced {len(LOOKS)} looks in {BACKGROUND}/Looks")
