"""Exports every animation sequence under ANIM_FOLDER to FBX with its preview mesh in OUTPUT_FOLDER, one subfolder per class.

Run via mcp-unreal execute_script. Report written to Saved/export_anim_fbx.txt.
"""
import os
import traceback

import unreal

ANIM_FOLDER = "/Game/Characters/Anim/ClassBadge"
SAVED_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
OUTPUT_FOLDER = SAVED_DIR + "ClassBadgeAnimFbx/"
REPORT = SAVED_DIR + "export_anim_fbx.txt"


def export_fbx(sequence, filename):
    task = unreal.AssetExportTask()
    task.object = sequence
    task.filename = filename
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    options = unreal.FbxExportOption()
    options.set_editor_property("export_preview_mesh", True)
    task.options = options
    return unreal.Exporter.run_asset_export_task(task)


LOG = []
try:
    assets = unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(ANIM_FOLDER, recursive=True)
    for asset in assets:
        if str(asset.asset_class_path.asset_name) == "AnimSequence":
            package = str(asset.package_name)
            folder = OUTPUT_FOLDER + package.split("/")[-2] + "/"
            os.makedirs(folder, exist_ok=True)
            done = export_fbx(unreal.load_asset(package), folder + str(asset.asset_name) + ".fbx")
            LOG.append("{}: {}".format(package, "ok" if done else "EXPORT FAILED"))
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
