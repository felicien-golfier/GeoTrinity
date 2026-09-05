"""Create ZoomAction and wire it to the mouse wheel — the input end of AGeoGameCamera's zoom.

The C++ side is already complete: UGeoInputComponent::BindInput binds ZoomAction on
ETriggerEvent::Triggered and hands the float to AGeoGameCamera::AddZoomInput, which does
`TargetZoom -= Wheel * ZoomWheelStep`. Only the three asset-side pieces are missing.

Axis1D, not Axis2D. EKeys::MouseWheelAxis is registered Axis1D and ZoomFromInput reads
Instance.GetValue().Get<float>(); an Axis2D action would hand that read a Vector2D and the wheel
would come back as zero every notch.

NO Negate modifier. Wheel up reports positive, AddZoomInput subtracts it from TargetZoom, and a
smaller OrthoWidth is a closer camera — so the raw axis already reads as "wheel up zooms in".
Negating here would invert it, which is why the mapping is left bare.

The mapping lands on BP_GeoInputMapping, the context BP_GeoPlayerController pushes. The action
pointer lands on the "Geo Input Component" template of BP_GeoPlayableCharacter, beside MoveAction and
LookAction: AGeoCharacter creates that component as a native subobject, so the Blueprint is where its
per-project values live.

UInputMappingContext::Mappings is deprecated in 5.7 in favour of DefaultKeyMappings.Mappings, so the
mapping goes through MapKey/UnmapAllKeysFromAction rather than assigning an array — those write
whichever array the engine currently owns and survive the next migration.

Idempotent: an existing action is reused, and its wheel mapping is cleared before being re-added so a
second run cannot stack duplicates.
"""
import unreal

ACTION_DIR = "/Game/Input/InputActions"
ACTION_NAME = "ZoomAction"
ACTION_PATH = f"{ACTION_DIR}/{ACTION_NAME}"
CONTEXT_PATH = "/Game/Input/BP_GeoInputMapping"
CHARACTER_BP_PATH = "/Game/Characters/Playable/BP_GeoPlayableCharacter"
WHEEL_KEY = "MouseWheelAxis"

at = unreal.AssetToolsHelpers.get_asset_tools()


def save(asset):
    """Write asset to disk, or fail loudly.

    save_asset defaults to only_if_is_dirty and reports the outcome only through a return value, so a
    freshly created asset whose package never got flagged dirty writes NOTHING and says nothing.
    """
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), \
        f"{asset.get_path_name()}: would not save"


def input_component(blueprint):
    """The UGeoInputComponent template of a character Blueprint, matched by class — the native
    subobject is named "Geo Input Component", spaces and all, which is not worth matching on."""
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = subsystem.k2_find_subobject_data_from_handle(handle)
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
        if isinstance(component, unreal.GeoInputComponent):
            return component
    return None


# --- ZoomAction ----------------------------------------------------------------------------------
if unreal.EditorAssetLibrary.does_asset_exist(ACTION_PATH):
    action = unreal.EditorAssetLibrary.load_asset(ACTION_PATH)
else:
    action = at.create_asset(ACTION_NAME, ACTION_DIR, unreal.InputAction, unreal.InputAction_Factory())
assert action, f"{ACTION_PATH}: would not create"

action.set_editor_property("ValueType", unreal.InputActionValueType.AXIS1D)
value_type = action.get_editor_property("ValueType")
assert value_type == unreal.InputActionValueType.AXIS1D, \
    f"{ACTION_NAME}: ValueType would not take, got {value_type}"
save(action)
# The context and the character Blueprint are about to take hard references, so the action has to be
# on disk before that — a reference to an unsaved package resolves to null on the next load.
assert unreal.EditorAssetLibrary.does_asset_exist(ACTION_PATH), f"{ACTION_NAME}: saved but is not on disk"

# --- BP_GeoInputMapping --------------------------------------------------------------------------
context = unreal.EditorAssetLibrary.load_asset(CONTEXT_PATH)
assert context, f"{CONTEXT_PATH}: would not load"

# FKey rejects constructor arguments in Python — build it empty and set the name.
wheel = unreal.Key()
wheel.set_editor_property("KeyName", WHEEL_KEY)

context.unmap_all_keys_from_action(action)
context.map_key(action, wheel)

# GetMappings() is not a UFUNCTION, so read the array it wraps: 5.7 moved it under DefaultKeyMappings.
mappings = context.get_editor_property("DefaultKeyMappings").get_editor_property("Mappings")
mapped = [str(m.get_editor_property("Key").get_editor_property("KeyName"))
          for m in mappings if m.get_editor_property("Action") == action]
assert mapped == [WHEEL_KEY], f"{CONTEXT_PATH}: wheel mapping would not take, got {mapped}"
save(context)

# --- BP_GeoPlayableCharacter ---------------------------------------------------------------------
character_bp = unreal.EditorAssetLibrary.load_asset(CHARACTER_BP_PATH)
assert character_bp, f"{CHARACTER_BP_PATH}: would not load"

component = input_component(character_bp)
assert component, f"{CHARACTER_BP_PATH}: no UGeoInputComponent template"
component.set_editor_property("ZoomAction", action)

assigned = component.get_editor_property("ZoomAction")
assert assigned and assigned.get_path_name() == action.get_path_name(), \
    f"BP_GeoPlayableCharacter: ZoomAction would not take, got {assigned}"
unreal.BlueprintEditorLibrary.compile_blueprint(character_bp)
save(character_bp)

print(f"OK {ACTION_PATH} ({value_type}) -> {WHEEL_KEY} on {CONTEXT_PATH}; assigned on {CHARACTER_BP_PATH}")
