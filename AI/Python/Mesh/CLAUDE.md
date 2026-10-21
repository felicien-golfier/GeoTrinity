# AI/Python/Mesh

Mesh toolkit and uncategorised meshes. Category meshes and rigs sit with their category: `Cat/Mesh`, `Gem/`, `Badge/Mesh`, `Boss/{Hex,Star}/Mesh`. The rules below apply to all of them.

- Winding: front face = clockwise seen from outside. Generators outline counter-clockwise and reverse in their one triangle helper. Check a new mesh against the engine cube: a top-face triangle's edge cross product points down.
- Static mesh writes, any script (generators, material slot edits): through `static_mesh_asset.py` only. `save_static_mesh` for a generator (full build), `load_mesh` / `save_mesh` for an edit: compile waited, fast-build flag cleared, built bounds checked against the vertices before the save. Never `build_from_static_mesh_descriptions` or a bare `save_loaded_asset` on a mesh. Rewriting a mesh can change its shading (computed normals, a material that relied on none): capture it in the level that uses it afterwards (`AI/MCP/MCP_Preview.md`).

| Script | Does |
|---|---|
| `static_mesh_asset.py` | Toolkit (exec-loaded) for every static mesh write: `save_static_mesh(folder, name, desc, materials)`, `load_mesh(package)`, `save_mesh(asset)` |
| `generate_bomb_mesh.py` | Round bomb mesh sized to replace the pillar |
