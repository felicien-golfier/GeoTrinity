"""Compiles each candidate layer in one slot of a material's layer stack and reports every one's instruction counts.

The slot's own layer goes back in at the end and the material is saved, so a run leaves the asset as it found it.
Run through MCP execute_script against the editor world, outside PIE; results go to REPORT.
"""
import json
import traceback

import unreal

REPORT = unreal.Paths.project_saved_dir() + "GeoTrinity_LayerComparison.json"

toolkit_path = unreal.Paths.project_dir() + "AI/Python/Material/material_graph_authoring.py"
toolkit = {}
exec(compile(open(toolkit_path).read(), toolkit_path, "exec"), toolkit)


def compare(material_path, index, layer_paths):
    """(pixel, vertex) instruction counts of the material with each candidate layer in slot index."""
    material = toolkit["load"](material_path)
    stack = toolkit["find_layer_stack"](material).get_editor_property("default_layers")
    original = stack.get_editor_property("layers")[index]
    counts = {}
    for path in layer_paths:
        toolkit["replace_layer"](material, index, toolkit["load"](path))
        stats = unreal.MaterialEditingLibrary.get_statistics(material)
        counts[path] = [stats.get_editor_property("num_pixel_shader_instructions"),
                        stats.get_editor_property("num_vertex_shader_instructions")]

    toolkit["replace_layer"](material, index, original)
    toolkit["save"](material)
    return counts


if __name__ == "__main__":
    LAYERS = "/Game/Art/VFX/Background/Layers"
    report = {}
    try:
        report = compare("/Game/Art/VFX/Background/M_BackgroundLattice", 1,
                         [f"{LAYERS}/{name}" for name in ("ML_Wave_Rings", "ML_Wave_SoftRings", "ML_Wave_Spiral")])
    except Exception:
        report["error"] = traceback.format_exc()

    with open(REPORT, "w") as f:
        json.dump(report, f, indent=1)
