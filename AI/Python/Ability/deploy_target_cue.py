# GC_DeployTarget: the AGeoDeployTargetCue Blueprint marking where a charging deploy ability lands.
# Needs the build carrying AGeoDeployTargetCue and the GameplayCue.Ability.Deployable.Target tag. Report: Saved/claude/deploy_target_cue.txt
import traceback

import unreal

FOLDER = "/Game/AbilitySystem/GameplayCues"
NAME = "GC_DeployTarget"
PARENT_CLASS = "/Script/GeoTrinity.GeoDeployTargetCue"
CUE_TAG = "GameplayCue.Ability.Deployable.Target"
MESH = "/Engine/BasicShapes/Plane"
MATERIAL = "/Game/Art/VFX/Generic/Materials/MatInstances/MI_GeoShape_RingDash4"
MARKER_SCALE = 0.8  # the plane is 100 cm across
SPIN_DEGREES_PER_SECOND = 90.0
REPORT_PATH = unreal.Paths.project_saved_dir() + "claude/deploy_target_cue.txt"

REPORT = []


def log(*args):
    REPORT.append(" ".join(str(a) for a in args))


def subobject_handles(blueprint):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    return subsystem, subsystem.k2_gather_subobject_data_for_blueprint(blueprint)


def find_component(blueprint, component_class):
    subsystem, handles = subobject_handles(blueprint)
    for handle in handles:
        component = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(
            subsystem.k2_find_subobject_data_from_handle(handle))
        if isinstance(component, component_class):
            return component
    return None


def add_component(blueprint, component_class):
    """Existing component of that class, or a new one under the root."""
    component = find_component(blueprint, component_class)
    if component is None:
        subsystem, handles = subobject_handles(blueprint)
        params = unreal.AddNewSubobjectParams(parent_handle=handles[0], new_class=component_class,
                                              blueprint_context=blueprint)
        _, fail_reason = subsystem.add_new_subobject(params)
        if not fail_reason.is_empty():
            raise RuntimeError("add %s: %s" % (component_class.static_class().get_name(), fail_reason))
        component = find_component(blueprint, component_class)
    return component


def build():
    path = "%s/%s" % (FOLDER, NAME)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        blueprint = unreal.load_asset(path)
        log("reusing", path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("ParentClass", unreal.load_class(None, PARENT_CLASS))
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, FOLDER, unreal.Blueprint, factory)
        log("created", path)

    mesh = add_component(blueprint, unreal.StaticMeshComponent)
    mesh.set_editor_property("StaticMesh", unreal.load_asset(MESH))
    mesh.set_editor_property("OverrideMaterials", [unreal.load_asset(MATERIAL)])
    mesh.set_editor_property("RelativeScale3D", unreal.Vector(MARKER_SCALE, MARKER_SCALE, 1.0))
    mesh.set_collision_profile_name("NoCollision")
    mesh.set_editor_property("CastShadow", False)

    spin = add_component(blueprint, unreal.RotatingMovementComponent)
    spin.set_editor_property("RotationRate", unreal.Rotator(0.0, 0.0, SPIN_DEGREES_PER_SECOND))

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    cue_tag = unreal.GameplayTag()
    cue_tag.import_text('(TagName="%s")' % CUE_TAG)
    cdo = unreal.get_default_object(blueprint.generated_class())
    cdo.set_editor_property("GameplayCueTag", cue_tag)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)

    log("tag", cdo.get_editor_property("GameplayCueTag").export_text())
    log("mesh", mesh.get_editor_property("StaticMesh").get_name(), [m.get_name() for m in mesh.get_editor_property("OverrideMaterials")])
    log("spin", spin.get_editor_property("RotationRate"))


try:
    build()
except Exception:
    log(traceback.format_exc())

with open(REPORT_PATH, "w", encoding="utf-8") as handle:
    handle.write("\n".join(REPORT))
