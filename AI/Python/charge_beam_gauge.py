"""
Create a Widget Blueprint, build its tree via a C++ shim, and wire it to a WidgetComponent on an actor Blueprint.
Usage: run via MCP execute_script. Adjust the call at the bottom for different widgets.
"""
import unreal


def create_widget_blueprint(name, folder, parent_class_path):
    full_path = f"{folder}/{name}"
    existing = unreal.load_asset(full_path)
    if existing:
        unreal.log(f"Asset already exists, reusing: {full_path}")
        return existing
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.load_class(None, parent_class_path))
    wbp = asset_tools.create_asset(name, folder, unreal.WidgetBlueprint, factory)
    unreal.EditorAssetLibrary.save_loaded_asset(wbp)
    return wbp


def build_widget_tree(shim_class, method_name, wbp, *args):
    cdo = shim_class.get_default_object()
    getattr(cdo, method_name)(wbp, *args)


def list_blueprint_subobjects(bp):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    for i, h in enumerate(handles):
        data = subsystem.k2_find_subobject_data_from_handle(h)
        obj  = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        unreal.log(f"[{i}] {type(obj).__name__} | {obj.get_name()}")
    return handles


def wire_widget_component(actor_bp_path, wbp, comp_index, relative_location, draw_size):
    bp = unreal.load_asset(actor_bp_path)
    handles = list_blueprint_subobjects(bp)
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    comp_data = subsystem.k2_find_subobject_data_from_handle(handles[comp_index])
    comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(comp_data)
    comp.set_editor_property("widget_class",      wbp.generated_class())
    comp.set_editor_property("relative_location", relative_location)
    comp.set_editor_property("draw_size",         draw_size)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)


# --- ChargeBeam gauge ---
def create_charge_beam_gauge():
    wbp = create_widget_blueprint(
        "WBP_ChargeBeamGauge",
        "/Game/HUD/ChargeBeam",
        "/Script/GeoTrinity.GeoChargeBeamGaugeWidget",
    )
    build_widget_tree(unreal.GeoWidgetBuilderUtil, "build_charge_beam_gauge_widget", wbp, 0.6, 0.7)
    wire_widget_component(
        "/Game/Characters/Playable/BP_GeoPlayableCharacter",
        wbp,
        comp_index=4,  # ChargeBeamGaugeComponent
        relative_location=unreal.Vector(-150.0, 0.0, 0.0),
        draw_size=unreal.IntPoint(15, 80),
    )
    unreal.log("charge_beam_gauge.py: done.")


create_charge_beam_gauge()
