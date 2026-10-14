# AI/Python/Material

Materials, material functions, parameter collections. Docs: `AI/Materials.md`, `AI/MCP/MCP_Material.md`.

`material_graph_authoring.py` is the toolkit the builders import (in-place rebuilds, asserted wiring, function / layer pins, parameters, layer stacks).

| Script | Does |
|---|---|
| `make_generic_material_functions.py` | Shape distances, polar, triangle cells, strokes, glows, Sierpinski, Lissajous, clock wipe |
| `make_color_pattern_functions.py` | Meaning-colour patterns (zigzag, quads, stripes) + `MF_MeaningColors` over `MPC_MeaningColors`; `PATTERN` picks |
| `make_pulse_circle_material.py` | `M_PulseCircle` zone disc from functions |
| `make_beam_and_telegraph_materials.py` | `M_PulseBeam`, `M_ZoneIndicatorRay`, `M_ZoneIndicator` |
| `make_arena_rail_material.py` | Arena border rail: core line, halo, travelling quads; instance per arena |
| `make_class_badge_materials.py` | Badge body material: logo masks, instance per class, wired to class data and meshes |
| `compare_material_layers.py` | Compile candidate layers in one slot, report instruction counts |
| `dry_run_material_build.py` | Run builders outside the editor against a stand-in, checking every pin |
| `read_material_text.py` | Export a material as text, read lines naming given properties |

## Background/ — floor
| Script | Does |
|---|---|
| `make_background_lattice_material.py` | Floor triangle line art: layer stack, glow blend, pulse collection |
| `make_background_looks.py` | Floor glow looks, one layer each |
| `make_background_look_instances.py` | One material instance per floor look |
