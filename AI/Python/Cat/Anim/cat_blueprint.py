"""BP_Cat: the Blueprint child of AGeoCat placed in the Tutorial; carries SKM_Cat, ABP_Cat and the idle variants it plays
while it stands (the other montages are the encounter's: GeoCatMontageAction).

Run AFTER cat_idles.py and cat_anim_blueprint.py, via mcp-unreal execute_script (needs a build
with AGeoCat). Re-runnable: an existing BP_Cat is updated in place. Report written to AI/Output/cat_blueprint.json.
"""
import json

import unreal

FOLDER = "/Game/Actors/Cat"
NAME = "BP_Cat"
PARENT_CLASS = "/Script/GeoTrinity.GeoCat"
MESH = "/Game/Art/Meshes/Cat/SKM_Cat"
ANIM_FOLDER = "/Game/Characters/Anim/Cat"
ANIM_BLUEPRINT = ANIM_FOLDER + "/ABP_Cat"
IDLE_VARIANTS = ("Toilet", "Groom", "Loaf", "LookAround", "Knead")  # AM_Cat_Idle_<Name>, cat_idles.py


def required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError("{} is missing — run its script first".format(path))
    return asset


def cat_blueprint():
    """Load BP_Cat, or create it as a child of AGeoCat."""
    existing = unreal.load_asset("{}/{}".format(FOLDER, NAME))
    if existing:
        return existing

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", unreal.load_class(None, PARENT_CLASS))
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, FOLDER, unreal.Blueprint, factory)


def main():
    result = {}
    try:
        blueprint = cat_blueprint()
        if blueprint is None:
            raise RuntimeError("could not create {}".format(NAME))
        defaults = unreal.get_default_object(blueprint.generated_class())
        mesh = defaults.get_editor_property("Mesh")
        mesh.set_editor_property("skeletal_mesh_asset", required(MESH))
        mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
        mesh.set_editor_property("anim_class", required(ANIM_BLUEPRINT).generated_class())
        defaults.set_editor_property(
            "IdleVariants", [required("{}/AM_Cat_Idle_{}".format(ANIM_FOLDER, name)) for name in IDLE_VARIANTS])
        # Instances copy only the defaults the last compile recorded.
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)

        defaults = unreal.get_default_object(blueprint.generated_class())
        mesh = defaults.get_editor_property("Mesh")
        result["blueprint"] = blueprint.get_path_name()
        result["mesh"] = mesh.get_editor_property("skeletal_mesh_asset").get_path_name()
        result["anim_class"] = str(mesh.get_editor_property("anim_class"))
        result["idle_variants"] = [variant.get_name() for variant in defaults.get_editor_property("IdleVariants")]
        result["ok"] = True
    except Exception as exc:  # noqa
        import traceback
        result["ok"] = False
        result["error"] = str(exc)
        result["trace"] = traceback.format_exc()
    with open((unreal.Paths.project_dir() + "AI/Output/") + "cat_blueprint.json", "w") as f:
        json.dump(result, f, indent=2)


main()
