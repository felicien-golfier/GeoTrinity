"""Imports image files as textures, replacing same-named assets, with the sRGB and compression they are sampled with.

Run via execute_script. Re-runnable: an existing texture is reimported in place.
"""
import unreal


def import_texture(file_path, destination_folder, asset_name, srgb=True,
                   compression=unreal.TextureCompressionSettings.TC_DEFAULT):
    """A mask or data texture takes srgb False; its sampler in a material must match that setting."""
    task = unreal.AssetImportTask()
    task.filename = file_path
    task.destination_path = destination_folder
    task.destination_name = asset_name
    task.replace_existing = True
    task.automated = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(f"{destination_folder}/{asset_name}")
    assert texture, f"{file_path}: did not import"
    texture.set_editor_property("srgb", srgb)
    texture.set_editor_property("compression_settings", compression)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False), f"{asset_name}: would not save"
    return texture


if __name__ == "__main__":
    import_texture(unreal.Paths.project_dir() + "GamePictures/tank_NB.png", "/Game/Characters/Meshes/Class/Materials",
                   "T_SquareBadge_Mask", srgb=False)
