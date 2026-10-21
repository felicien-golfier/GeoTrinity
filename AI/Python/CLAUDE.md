# AI/Python — automation scripts

Scripts grouped by what they touch, one folder per topic; each folder's `CLAUDE.md` lists its scripts. Run rules (re-runnable, saving, reports): `AI/MCP/CLAUDE.md`.

| Folder | Holds | Topic doc |
|---|---|---|
| **Categories** (a feature or entity, all its scripts) | | |
| `Tutorial/` | `Level/` map, data, encounters; `UI/` menu + HUD; `Runtime/` flow checks | `AI/Tutorial/` |
| `Cat/` | The Tutorial Cat: `Mesh/`, `Anim/`, materials | `AI/Tutorial/CatAnimation.md` |
| `Gem/` | Gem catalog, Core passives, Gems menu pages, CSV ↔ design-doc sync, gem meshes + materials | `Source/GeoTrinity/Public/Gem/CLAUDE.md`, skills `gem-csv-to-doc`, `gem-doc-to-csv` |
| `Badge/` | Class badges: `Mesh/` (+ turret), `Anim/`, material | `MCP_Animation.md` |
| `Boss/` | Hex / star bosses: `Hex/{Mesh,Anim}`, `Star/{Mesh,Anim}`, shared `Anim/`, fight loop | `MCP_Animation.md` |
| **Types** (no category: generic or cross-cutting) | | |
| `Ability/` | Ability BPs, ability catalog, StateTrees, Core passives | `MCP_NewEnemyAbility.md`, `MCP_StateTree.md` |
| `Anim/` | Skeletal animation toolkit and maintenance | `MCP_Animation.md` |
| `Asset/` | Generic asset editing, imports, saving | `MCP_Blueprint.md` |
| `Audio/` | References, synthesis, fitting, kits, boss scores (`uv`, outside the editor) | `AI/SoundCreation.md` |
| `Level/` | Generic level content and settings | `MCP_Settings.md` |
| `Material/` | Generic materials, functions, parameter collections | `AI/Materials.md` |
| `Mesh/` | Mesh toolkit, uncategorised meshes | — |
| `Niagara/` | Systems and emitter stacks | `AI/VFX.md` |
| `Runtime/` | Driving a running session or the editor world | `MCP_PIE.md`, `MCP_Preview.md` |
| `UI/` | Generic widget Blueprints and trees | `AI/UI.md`, `MCP_UI.md` |
| `Video/` | Footage scan and edit-list render (`uv`, outside the editor) | `AI/MCP/Video/CLAUDE.md` |
| `Tools/` | Harness hooks (`token_meter.py`) | — |

## Conventions
- A script another imports is loaded by its project-relative path: moving one means fixing those literals.
- Tunables in a block at the top of each file.
- Placement: **category first, then type.** A script about one feature or entity (Tutorial, Cat, Gem, Badge, Boss/Hex …) lives in that category's folder; inside it, a type subfolder named like the type folders (`Mesh`, `Anim`, `Level`, `UI`, `Runtime`, `Niagara`) as soon as the category has ≥ 2 scripts of that type, else flat at the category root. A script with no category goes in its type folder. A new category = ≥ 2 scripts of one subject, its own `CLAUDE.md` (table per type); a script that serves two categories stays with the one that owns its output.
- A toolkit others load stays at its type folder's root (`Anim/anim_sequence_authoring.py`, `Mesh/static_mesh_asset.py`, `UI/asset_guard.py`, `Material/material_graph_authoring.py`, `Niagara/niagara_stack_edit.py`). A type folder's own sub-grouping (`Niagara/Electric`, `Material/Background`) stays.
- `Audio/` stays one folder: its scripts import each other as siblings and run with `uv`; kits and boss scores are not split by category.
- New script: the folder above, plus one row in that folder's `CLAUDE.md`.
- Only scripts that run again live here. One-off change: Epic MCP tools; a finished one-shot script is deleted (`AI/MCP/CLAUDE.md`, "Script rules").
