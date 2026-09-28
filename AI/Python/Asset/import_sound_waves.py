"""Imports every WAV in each source folder as a Sound Wave, SFX_Name.wav becoming SW_Name, replacing assets of that name.

Run via mcp-unreal execute_script. Re-runnable: an existing Sound Wave is reimported in place. A Sound Wave in a
destination package whose source file is gone is deleted, unless something still references it.
Report written to Saved/import_sound_waves.txt.
"""
import glob
import os
import traceback

import unreal

# (source folder under the project, destination package)
FOLDERS = [
    ("SourceArt/Audio/Metal", "/Game/Art/SFX/Metal"),
    ("SourceArt/Audio/Machine", "/Game/Art/SFX/Machine"),
    ("SourceArt/Audio/Mech", "/Game/Art/SFX/Mech"),
    ("SourceArt/Audio/HexBossIntro", "/Game/Art/SFX/Enemy/HexBoss"),
]
FILE_PREFIX = "SFX_"
ASSET_PREFIX = "SW_"
REPORT = unreal.Paths.project_saved_dir() + "import_sound_waves.txt"


def referencers(package):
    """Packages referencing this one; their folders are rescanned first, as the registry keeps a re-saved package's
    old references."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions()
    stale = registry.get_referencers(package, options) or []
    if stale:
        registry.scan_paths_synchronous(sorted({str(name).rsplit("/", 1)[0] for name in stale}), True)

    return [str(name) for name in registry.get_referencers(package, options) or []]


LOG = []
try:
    tasks = []
    sourced = set()
    for folder, destination in FOLDERS:
        for wav in sorted(glob.glob(os.path.join(unreal.Paths.project_dir(), folder, FILE_PREFIX + "*.wav"))):
            name = os.path.splitext(os.path.basename(wav))[0].replace(FILE_PREFIX, ASSET_PREFIX, 1)
            sourced.add("{}/{}".format(destination, name))
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", os.path.abspath(wav))
            task.set_editor_property("destination_path", destination)
            task.set_editor_property("destination_name", name)
            task.set_editor_property("replace_existing", True)
            task.set_editor_property("automated", True)
            task.set_editor_property("save", True)
            tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    for task in tasks:
        path = "{}/{}".format(task.get_editor_property("destination_path"), task.get_editor_property("destination_name"))
        sound = unreal.load_asset(path)
        LOG.append("{} -> {} {}".format(os.path.basename(task.get_editor_property("filename")), path,
                                        "{:.3f}s".format(sound.get_editor_property("duration")) if sound
                                        else "NOT IMPORTED"))

    for folder, destination in FOLDERS:
        for asset in unreal.EditorAssetLibrary.list_assets(destination, recursive=False):
            package = asset.split(".")[0]
            if package.rsplit("/", 1)[1].startswith(ASSET_PREFIX) and package not in sourced:
                users = referencers(package)
                if users:
                    LOG.append("{} has no source, kept: referenced by {}".format(package, ", ".join(users)))
                else:
                    LOG.append("{} has no source, {}".format(
                        package, "deleted" if unreal.EditorAssetLibrary.delete_asset(package) else "NOT DELETED"))
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
