# AI/Python/Mesh

Procedural static meshes and rigging.

- Winding: front face = clockwise seen from outside. Generators outline counter-clockwise and reverse in their one triangle helper. Check a new mesh against the engine cube: a top-face triangle's edge cross product points down.

| Script | Does |
|---|---|
| `generate_bomb_mesh.py` | Round bomb mesh sized to replace the pillar |
| `generate_class_badge_meshes.py` | Three class logo silhouettes extruded per class; Triangle body thins to its point |
| `generate_hex_boss_mesh.py` | Hex boss body (three concentric hexagons) + rig data |
| `generate_hex_mine_mesh.py` | Hex boss mine: flat hexagon, six spikes alternating long / short (turret cannon shape), core hexagon in its own `Core` slot for the blinking material |
| `generate_hex_turret_mesh.py` | Hex boss turret: flat hexagon + hexagonal-section cannon along +X (firing direction), separate prism for later animation |
| `generate_triangle_turret_mesh.py` | Triangle class turret: flat triangular body (tip +X) with a layered round hub, long triangular-section cannon out of the tip (dark bands, muzzle block with two stripes), rear block, two grey side cannons at the back corners; slots `Color` / `Cannon` / `CannonBand` / `Legs` |
| `generate_star_mesh.py` | Star boss body: eight points, counter-points, star-shaped heart hole |
| `rig_hex_boss.py` | Hex body → skeletal mesh on a new skeleton |
| `rig_star.py` | Rebuild `SKM_Star` on `SK_Star`'s hierarchy, weighted so existing clips play unchanged |
| `rig_class_badges.py` | Rig badges: body bottom layer, parts on own bones under a top layer, fire sockets, Square keyhole socket |
