---
name: sync-tutorial-scripts
description: Read the whole Tutorial as it is in the editor (map, hierarchy, encounters, assets) and rewrite the AI/Python/Tutorial scripts so replaying them rebuilds exactly that
---

# Sync Tutorial Scripts

The editor is the truth, the scripts follow (`AI/Python/Tutorial/CLAUDE.md`, "Read the scene first"). Goal: running the Tutorial scripts on a clean project gives exactly what is in the editor now: every actor, transform, folder, tag, volume extent, encounter step, and every asset the tutorial owns. Work autonomously; the user asked for this sync, so scripts are edited without the workflow gate. No PIE, no focus, no editor launch/quit (`CLAUDE.md` "Rules"): the user's editor must already be open with the bridge up (`AI/MCP/MCP_LiveCodingAndConnect.md`); if not, say so and stop.

## Steps

1. **Read the docs.** `AI/Python/Tutorial/CLAUDE.md`, `AI/MCP/CLAUDE.md` (Script rules, Hazards), `AI/Tutorial/Flow.md`, `AI/Tutorial/Steps.md`, `AI/CodingStyle.md` (about to write code).

2. **Dump the scene** through `python AI/Python/Runtime/run_via_bridge.py <script>`:
   - `Level/scene_dump.py` → `AI/Output/tutorial_scene.txt`: every actor of `/Game/Maps/Tutorial` (class, label, folder, parent, tags, transforms, components, properties off the class default).
   - `Level/encounters_diff.py` → `AI/Output/encounters_diff.txt` + `encounters_scene.txt`: every room's steps and the spawn points, scene (`-`) vs script (`+`).
   Never save, move or rename anything in the scene.

3. **List what changed in assets.** `git status --short Content/ Config/ Data/` and `git diff --stat` show the assets touched since the last commit: tutorial-owned ones (`Content/Maps/Tutorial.umap`, `Content/AI/Tutorial`, `Content/AbilitySystem/Abilities/Enemy/{Training,TutorialMiniboss,TutorialWaves}`, `CT_Tutorial`, `DT_CatLine`, tutorial widgets, `BP_Cat`, new tags) must each be read back with `mcp-epic` property/Blueprint/table tools (or an inline `execute_script` read) and compared with the script that owns them (table in `AI/Python/Tutorial/CLAUDE.md`). An asset the scripts do not own yet: add it to the script of its topic, or a new script by the placement rules (`AI/Python/CLAUDE.md`). An asset deleted in the editor: delete its creation from the script and add it to that script's old-layout clean-up.

4. **Diff scene vs scripts, per script**, in this order:
   - `tutorial_map.py`: room count and order (`ROOMS`), every geometry constant (room halves, walls, gap, void, pads, `PlayerStart`s, entrance points, camera / arena / trigger volume extents, world settings, game mode defaults), outliner folders and hierarchy (`Tutorial/Rooms/<NN_Room>/<Category>`, attach parents), tags, actors present in the scene but not placed by any script (add), placed by a script but gone from the scene (remove, plus `REMOVED_ROOMS` if a rebuild must clean it).
   - `tutorial_encounters.py`: every `-` line of `encounters_diff.txt`; points, zone boxes, arenas (`BossClass`, `BossSetup`), `AGeoEncounter` steps, Cat staging, jump starts.
   - `tutorial_assets.py`, `tutorial_waves.py`, `tutorial_miniboss.py`, `training_room.py`, `UI/tutorial_*.py`: every value read in step 3.
   A value that is hand-placed (a free location, a table of points) becomes a literal in a named table at the top of the script, with the scene's exact numbers (3 decimals of the dump, not rounded further). A value that is derived (room centre, inset) stays a formula only if it reproduces the scene's number exactly; otherwise store the scene's number.

5. **Edit the scripts** so the `+` side equals the `-` side. Smallest edit that makes it exact; keep each file's style, tunables block at the top, one-line docstrings. A new action property → add it to `PROPS` in `encounters_diff.py` (and to `scene_dump.py` if it is not editor-visible by default). Hand-edit ledger (`AI/Python/UI/asset_values.json`): a value the script now writes equal to the scene needs no ledger change; never edit the ledger to hide a difference.

6. **Prove it.** Re-run `encounters_diff.py`: every section empty. Re-run `scene_dump.py` and compare with step 2's dump only when a script was re-executed. To prove the map script itself, rebuild into a throwaway map copy (never overwrite the user's `Tutorial.umap`: ask first) and diff its dump against the real one; if that is not possible, say the map script was reviewed, not replayed. Python changed in `Source`-free files only, so no build is needed (the tutorial gate concerns `Source/`, `Tutorial.umap` and tutorial content: do not modify those).

7. **Update docs** that drifted (`AI/Python/Tutorial/CLAUDE.md` rows, `AI/Tutorial/*.md` for design facts) per `CLAUDE.md` "Where a fact goes".

## Output
Per script: what was brought in line (one line each), what could not be (and why), the proof that was run, and anything in the scene a script cannot express yet.
