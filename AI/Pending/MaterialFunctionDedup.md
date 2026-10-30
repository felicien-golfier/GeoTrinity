# MaterialFunctionDedup — pending

Goal: no material function repeats a block of nodes, and every material graph reads like `M_MAskedArea`: functions where a block is needed, room where it is needed, no wire back, no box over another, shared values as variables.
Priority: 3
Docs written: `AI/Materials.md` ("Writing a function", "Keeping a graph readable"), `AI/Python/Material/CLAUDE.md` ("Layout"), `AI/MCP/MCP_Material.md` ("Graph organisation").

Done (this pass): dedup (`MF_IsWholeNumber`, `MF_ZeroOneToSigned`, `MF_SafeSpotSlotCut`), toolkit layout (tight per-node sizes, mask-of-mask merged, unused nodes deleted), coordinates removed from every builder, generic / colour-pattern / pulse-circle / beam scripts rebuilt in the open editor , `MF_BeamShape` deleted (no referencers).

## Code (to implement) — variables
Size (measured by the dry run): 75 graphs, 40 of them carry rails, 427 rail reroutes in all (`MF_TriangleCell` 50, `MF_Wave_Whirl` 33, `MF_SafeSpotCut` 31, `MF_Wave_PolygonRings` 30, `MF_RailFrame` 29). A variable replaces each rail by one declaration and one usage per reader: no builder script changes, the toolkit decides.
- [ ] `Source/GeoTrinityEditor` — a shim linking a `MaterialExpressionNamedRerouteUsage` to its `...Declaration` (`Declaration`, `DeclarationGuid`, `VariableGuid` are not reflected to Python). Build with the editor closed (`Tools\*.bat`, `GEO_NO_PAUSE=1`), then `GeoTrinity.Tutorial.FullRun` (tutorial gate); pattern in `AI/MCP/MCP_EditorUtility.md`; docs of the editor module first.
- [ ] `AI/Python/Material/material_graph_authoring.py` — `graph.variable(name, source)` / `graph.read(name)` on that shim; `place_nodes` lets a variable replace a reroute rail (the declaration sits right of its source, each usage left of its reader); `AI/Python/Material/CLAUDE.md` "Layout" and `AI/MCP/MCP_Material.md` updated in the same change

## Scripts
- [ ] Rebuild with the new layout, headless or over the bridge (their instruction counts must not move): `make_arena_rail_material.py`, `Background/make_background_lattice_material.py`, `Background/make_background_looks.py`, `Background/make_background_look_instances.py` after them, `Cat/make_cat_materials.py`, `Gem/make_gem_materials.py`, `Badge/make_class_badge_materials.py` (dry-run ok except the badge one, whose stand-in lacks `unreal.PlayerClass`)

## Verify
- [ ] User: open `MF_SafeSpotCut` and `M_ZoneLayer` and judge the layout against `M_MAskedArea`; node size estimates in the toolkit tuned if boxes touch or gaps are too wide
- [ ] Lateral wave lanes and `Disc` hole still cut (open items of `BeamSafeSpotMask.md` "Verify")
