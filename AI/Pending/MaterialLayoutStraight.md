# MaterialLayoutStraight — pending

Goal: generated material graphs read cleanly: root node at the far right, wires straight and level with their target pin, few crossings, rails flat instead of stairs.
Priority: 1
Docs written: `AI/Python/Material/CLAUDE.md` ("Layout"), `AI/Materials.md` ("Keeping a graph readable").

## Scripts
- [ ] `AI/Python/Material/material_graph_authoring.py` `place_nodes` — set the material's root node position (`editor_x` / `editor_y` of the Material) after the last column, at the average height of its entering wires (it sits at the origin on top of the graph today); functions unchanged (outputs are in the last column).
- [ ] same — pin heights: helper `pin_y(box, index)` = `top + HEADER_HEIGHT + PIN_HEIGHT * index + PIN_HEIGHT / 2`; a box's wanted top = target pin y − its own output pin y (replaces the centre-to-centre `row` average); several targets → level with the closest pin.
- [ ] same — column order: sort by target pin height, then barycentre sweeps (left-to-right, right-to-left, a few passes); keep the order with the fewest `segments_cross` (use pin positions in `check_layout`).
- [ ] same — `route_through_rails` / `reroute`: all reroutes of one rail get the source pin's height (nearest free gap between rows if a box blocks it); only the last hop goes to the target pin. Reroute `top` is set from the rail height, not packed with the nodes.
- [ ] same — `check_layout`: crossings from pin positions, and log `B bends` after `C crossings` (format in the CLAUDE.md).
- [ ] `AI/Python/Material/dry_run_material_build.py` — the stand-in must record the root node position and reroute heights; assert root x > every other node, rail reroutes share one `y`.

## Verify
- [ ] Rebuild `M_ZoneLayer`, `M_TeleportPortal`, `MF_TelegraphColors` / `MF_MeaningColors` (the screenshots that prompted this): root box on the right, no S-curve stairs, `LAYOUT::` crossings and bends lower than before.
- [ ] Dry run (`dry_run_material_build.py`) green on every builder.
