# AI/Python — automation scripts

Scripts grouped by what they touch, one folder per topic; each folder's `CLAUDE.md` lists its scripts. Run rules (re-runnable, saving, reports): `AI/MCP/CLAUDE.md`.

| Folder | Holds | Topic doc |
|---|---|---|
| `Ability/` | Ability BPs, ability catalog, StateTrees, Core passives | `MCP_NewEnemyAbility.md`, `MCP_StateTree.md` |
| `Anim/` | Skeletal animation toolkit; `Badge/` class-badge clips, `Boss/` hex / star clips | `MCP_Animation.md` |
| `Asset/` | Generic asset editing, imports, saving | `MCP_Blueprint.md` |
| `Audio/` | References, synthesis, fitting, boss scores (`uv`, outside the editor) | `AI/SoundCreation.md` |
| `Gem/` | Gem catalog, Core passives, Gems menu pages, CSV ↔ design-doc sync | `Source/GeoTrinity/Public/Gem/CLAUDE.md`, skills `gem-csv-to-doc`, `gem-doc-to-csv` |
| `Level/` | Level content and settings | `MCP_Settings.md` |
| `Material/` | Materials, functions, parameter collections | `AI/Materials.md` |
| `Mesh/` | Procedural static meshes, rigging | — |
| `Niagara/` | Systems and emitter stacks | `AI/VFX.md` |
| `Runtime/` | Driving a running session or the editor world | `MCP_PIE.md`, `MCP_Preview.md` |
| `UI/` | Widget Blueprints and trees | `AI/UI.md`, `MCP_UI.md` |
| `Video/` | Footage scan and edit-list render (`uv`, outside the editor) | `AI/MCP/Video/CLAUDE.md` |
| `Tools/` | Harness hooks (`token_meter.py`) | — |

## Conventions
- A script another imports is loaded by its project-relative path: moving one means fixing those literals.
- Tunables in a block at the top of each file.
- New script goes in the folder matching what it touches, plus one row in that folder's `CLAUDE.md`. A folder with ≥ 3 scripts of one kind gets a subfolder (`Anim/Badge`, `Anim/Boss`, `Niagara/Electric`, `Material/Background`); a toolkit others load stays at its folder's root.
- Only scripts that run again live here. One-off change: Epic MCP tools; a finished one-shot script is deleted (`AI/MCP/CLAUDE.md`, "Script rules").
