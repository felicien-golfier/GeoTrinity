# AI/Python/Material

Materials, material functions, parameter collections. Docs: `AI/Materials.md`, `AI/MCP/MCP_Material.md`. Category materials sit with their category: `Cat/`, `Gem/`, `Badge/`.

`material_graph_authoring.py` is the toolkit the builders import (in-place rebuilds, asserted wiring, function / layer pins, parameters, Custom HLSL nodes, layer stacks).

## Layout
A builder never gives a position: `node`, `op`, `mask`, `call`, `parameter`, `input`, `output` take none. `connect` only records the wire; `finish()` places every node, makes every wire, then compiles and saves.
- Columns: a node's column = its step in the longest chain, pushed right as far as its consumers allow. Every wire joins neighbouring columns, so none goes back or runs over a node.
- Straight wires, the main aim: a wire is straight when its two pins share a height. A node is placed so its output pin is level with the input pin it feeds (pin height = `HEADER_HEIGHT` + `PIN_HEIGHT` × pin index; never box centres). A node feeding several pins is level with the closest one; the others bend once, just before their target. A column is stacked around those heights, none over another; a chain of nodes stays on one horizontal line.
- Few crossings: a column is ordered by the height of the pins its nodes feed (a wire ending higher comes from a node placed higher), refined by barycentre sweeps in both directions; `finish()` keeps the order with the fewest crossings.
- Rails: a value read further than the next column travels plain reroutes, one per column, shared by every pin that reads it. All reroutes of a rail share one height (the source pin's, or the nearest free gap between rows when a box is in the way): the rail is a flat line; only its last hop leaves that height, to the target pin. Never stairs or S curves between columns.
- Root: the material's root node (the big `M_…` box; a function has none) is placed after the last column, at the average height of the wires entering it; never at the origin, where it covers the graph.
- Node sizes are estimates per node: width from its title and pin names, height from its pin count and preview (`CHARACTER_WIDTH`, `PIN_HEIGHT`, `PREVIEW_HEIGHT`, … at the top of the toolkit); a column is its widest node plus a gap of `COLUMN_GAP` + `WIRE_GAP` per wire leaving it (up to `MAX_COLUMN_GAP`), rows are `ROW_GAP` apart. Tune those constants against the editor, not the positions. `finish()` asserts no two boxes overlap and no wire skips or goes back, and logs `LAYOUT::<asset>: N nodes, R reroutes, C crossings, B bends` (bend = a wire whose two pins differ in height).
- `fold(cls, sources)` chains one `Max` / `Min` / … over several sources, each node taking the result so far: the slots then follow each other left to right, one column each. Stacking them in one column (a tree) fans every shared value across all of them, unreadable until variables replace the rails.
- No useless box: `mask` of a mask builds one mask of the first source, and `finish()` deletes every node nothing reads (logged `unused nodes deleted`; a builder that logs it has a bug to fix). A function's pins and the material's outputs stay.
- A rail is the stand-in for a named reroute (variable, the model in `AI/Materials.md`): the usage → declaration link has no Python surface (`AI/MCP/MCP_Material.md`).
- Comment boxes are not scripted (same page).

| Script | Does |
|---|---|
| `make_generic_material_functions.py` | Shape distances, polar, triangle cells, strokes, glows, Sierpinski, Lissajous, clock wipe, `MF_ZeroOneToSigned`, `MF_IsWholeNumber` |
| `make_color_pattern_functions.py` | Meaning-colour patterns (zigzag, quads, stripes) + `MF_MeaningColors` and the telegraphs' `MF_TelegraphColors` over `MPC_MeaningColors` (also `StackBrightness`, `StackBrightnessMax`, `FillShade`, `OutlineWidth`); `PATTERN` picks |
| `make_zone_layer_materials.py` | `M_ZoneLayer` + its two instances, `MF_ZoneShapeDistance`, `MF_ZoneLayerCoverage`, `MF_ZoneMaskColors`, `MF_SafeSpotCut` (one `MF_SafeSpotSlotCut` call per slot) |
| `zone_layer_hlsl.py` | The zone shapes' HLSL and the coverage loop, one source for every Custom node of the zone layer |
| `make_arena_rail_material.py` | Arena border rail: core line, halo, quads travelling along the wall's local +X (`AI/ArenaWalls.md`, "Orientation"); one halo and one overlay instance for every wall |
| `compare_material_layers.py` | Compile candidate layers in one slot, report instruction counts |
| `dry_run_material_build.py` | Run builders outside the editor against a stand-in, checking every pin |
| `read_material_text.py` | Export a material as text, read lines naming given properties |

## Background/ — floor
| Script | Does |
|---|---|
| `make_background_lattice_material.py` | Floor triangle line art: layer stack, glow blend, pulse collection |
| `make_background_looks.py` | Floor glow looks, one layer each |
| `make_background_look_instances.py` | One material instance per floor look |
