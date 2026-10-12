# AI/Python — automation scripts

Scripts grouped by what they touch, one folder per topic; each folder's `CLAUDE.md` lists its scripts. Run rules (re-runnable, saving, reports): `AI/MCP/CLAUDE.md`.

| Folder | Holds | Topic doc |
|---|---|---|
| `Ability/` | Ability BPs, ability catalog, StateTrees, Core passives | `MCP_NewEnemyAbility.md`, `MCP_StateTree.md` |
| `Anim/` | Skeletal animation, montages, class badges, boss clips | `MCP_Animation.md` |
| `Asset/` | Generic asset editing, imports, saving, gem catalog | `MCP_Blueprint.md` |
| `Audio/` | References, synthesis, fitting, boss scores (`uv`, outside the editor) | `AI/SoundCreation.md` |
| `Data/` | Game data files ↔ design docs (plain Python) | skills `gem-csv-to-doc`, `gem-doc-to-csv` |
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
- New script goes in the folder matching what it touches, plus one row in that folder's `CLAUDE.md`.
