"""Imports every WAV in each source folder as a Sound Wave, SFX_Name.wav becoming SW_Name, replacing assets of that name.

Run via mcp-unreal execute_script. Re-runnable: an existing Sound Wave is reimported in place.
Report written to Saved/import_sound_waves.txt.
"""
import glob
import os
import traceback

import unreal

# (source folder under the project, destination package)
FOLDERS = [
    ("SourceArt/Audio/Metal", "/Game/Art/SFX/Metal"),
    ("SourceArt/Audio/HexBossIntro", "/Game/Art/SFX/Enemy/HexBoss"),
]
FILE_PREFIX = "SFX_"
ASSET_PREFIX = "SW_"
REPORT = unreal.Paths.project_saved_dir() + "import_sound_waves.txt"

LOG = []
try:
    tasks = []
    for folder, destination in FOLDERS:
        for wav in sorted(glob.glob(os.path.join(unreal.Paths.project_dir(), folder, FILE_PREFIX + "*.wav"))):
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", os.path.abspath(wav))
            task.set_editor_property("destination_path", destination)
            task.set_editor_property("destination_name",
                                     os.path.splitext(os.path.basename(wav))[0].replace(FILE_PREFIX, ASSET_PREFIX, 1))
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
except Exception:
    LOG.append(traceback.format_exc())

with open(REPORT, "w") as handle:
    handle.write("\n".join(LOG))
