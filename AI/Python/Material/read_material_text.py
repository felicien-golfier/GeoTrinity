"""Exports a material asset as text and returns the lines naming given properties, such as a constant-driven input.

A material input fed by a constant instead of a node has no Python property; its value sits in the text export.
Only the material asset itself is exported: exporting its editor-only-data subobject kills the editor.
Run via execute_script. Report written to Saved/read_material_text.txt.
"""
import unreal


def export_text(material_path, file_path):
    task = unreal.AssetExportTask()
    task.object = unreal.load_asset(material_path)
    assert task.object, f"{material_path}: missing"
    task.filename = file_path
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.exporter = unreal.ObjectExporterT3D()
    assert unreal.Exporter.run_asset_export_task(task), f"{material_path}: export failed"


def read_properties(material_path, names):
    """The exported lines of material_path that start with one of names, e.g. EmissiveColor=(UseConstant=True,...)."""
    file_path = unreal.Paths.project_saved_dir() + "material_export.t3d"
    export_text(material_path, file_path)
    with open(file_path) as handle:
        return [line.strip() for line in handle if line.strip().startswith(tuple(f"{name}=" for name in names))]


if __name__ == "__main__":
    lines = read_properties("/Game/Characters/Meshes/Cone/MAT_Cone_Alive", ("BaseColor", "EmissiveColor"))
    with open(unreal.Paths.project_saved_dir() + "read_material_text.txt", "w") as report:
        report.write("\n".join(lines))
