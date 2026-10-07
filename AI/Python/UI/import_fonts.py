"""
Import the menu fonts from SourceArt/Fonts as Font Face + Font asset pairs (<Name> and <Name>_Font) in
/Game/HUD/Assets/Fonts, the same pair the editor makes when its import dialog is answered Yes.
A Font's typefaces are not editable from Python, so the Font is left to the import dialog: run NON-automated and answer
"Yes" on each "Font Face Import Options" prompt (the call blocks the editor until every prompt is answered).
Usage: run via MCP execute_script. Re-run-safe (replaces the faces).
"""
import os
import unreal

SOURCE_DIR = os.path.join(unreal.Paths.project_dir(), "SourceArt", "Fonts")
DESTINATION = "/Game/HUD/Assets/Fonts"
FACES = [
    "ChakraPetch-Regular", "ChakraPetch-Medium", "ChakraPetch-SemiBold", "ChakraPetch-Bold",
    "IBMPlexMono-Regular", "IBMPlexMono-Medium", "IBMPlexMono-SemiBold",
]


def import_faces(names):
    tasks = []
    for name in names:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(SOURCE_DIR, name + ".ttf")
        task.destination_path = DESTINATION
        task.automated = False
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for name in names:
        font_path = DESTINATION + "/" + name + "_Font"
        if unreal.EditorAssetLibrary.does_asset_exist(font_path):
            unreal.EditorAssetLibrary.save_asset(font_path)


import_faces(FACES)
