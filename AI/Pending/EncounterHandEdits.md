# EncounterHandEdits — pending

Goal: a script never overrides fresh changes on any encounter or map (Tutorial for now, any later one too).
Priority: none — undecided; the user fears it slows the workflow. Pick up only when the user says so.
Docs written: none yet. `AI/Python/Tutorial/CLAUDE.md` ("Hand edits survive a re-run") still describes the older per-value ledger plan, which this replaces: rewrite it when picked up. Widget ledger (`asset_guard`) unchanged.

## Decisions
- Workflow gate, checked by Claude before launching a map-writing script; no hook, no code enforcement.
- Hash first: map unchanged since Claude's last run → no diff, script runs straight away.
- Map changed by hand → script does not run until the changes are carried into it (a single edit blocks the whole run).
- Unsaved map: save it first, then check.
- Snapshots and hashes not committed: `AI/Output/MapSnapshots/`. A pull with map changes is flagged once.
- Direct MCP changes count as the user's: no gate; they show up in the next diff. Before a direct MCP write, read the value and report if strangely different.
- No throwaway-map replay. Claude finishes the sync and does a full check before ending.

## Gate (before launching a script that writes a map)
1. Map dirty in the editor → save it.
2. Hash `Content/Maps/<Map>.umap`, compare with `AI/Output/MapSnapshots/<Map>.hash`. Same → run the script, go to 5.
3. Different → dump the level, diff against `<Map>` snapshot. Empty (a resave only) → update the hash, run the script, go to 5.
4. Differences → carry each into the script ("scene is the truth", skill `sync-tutorial-scripts`), then run the script.
5. After the run: dump again, compare with the dump before it: every line from step 4 still holds, and only intended lines moved. A hand change lost → fix the script, re-run. Then write the new snapshot + hash.

## Read map
- `AI/Python/Tutorial/Level/scene_dump.py` — current whole-level dump (`dir()`-based, can miss properties)
- `AI/Python/Tutorial/Level/encounters_diff.py` — steps dump (hand-kept `PROPS` list, can miss properties)
- `AI/Python/Tutorial/Level/tutorial_map.py`, `tutorial_encounters.py`, `training_room.py` — scripts that save a map
- `.claude/skills/sync-tutorial-scripts/SKILL.md` — the carry-into-script step
- `AI/Python/UI/asset_guard.py` — the widget ledger, stays as it is

## Scripts
- [ ] Probe: T3D level export (`unreal.AssetExportTask` on the world) through the bridge: holds every non-default property and instanced `Steps`, reads back identical between two exports? No → widen `scene_dump.py` + `encounters_diff.py` instead.
- [ ] `AI/Python/Level/map_guard.py` — `check(map)`: save if dirty, hash, dump + diff only on a hash change, report the differences. `commit(map)`: write snapshot + hash. Run through `AI/Python/Runtime/run_via_bridge.py`.
- [ ] List every other script that writes a map: confirm `level_settings.py`, `arena_walls.py`, `tutorial_miniboss.py`, `tutorial_waves.py`.
- [ ] Seed: sync scripts with the scene once (skill), then `commit` the Tutorial map and DraftMap.

## Docs
- [ ] `AI/Python/Tutorial/CLAUDE.md` — replace "Hand edits survive a re-run" and "Read the scene first" with the gate.
- [ ] `AI/Python/Level/CLAUDE.md` — `map_guard.py` row.
- [ ] `AI/MCP/CLAUDE.md` — "Hand edits" hazard: point to the gate for maps; read a value before a direct MCP write.
- [ ] Root `CLAUDE.md` — gate rule in "Rules".
- [ ] `.claude/skills/sync-tutorial-scripts/SKILL.md` — ledger line to the gate; `commit` once proven.
- [ ] `AI/Pending/PendingSweep.md` — drop the "can overwrite hand edits" warning.

## Verify
- [ ] No hand change: hash matches, script runs, no dump.
- [ ] Resave with no change: diff empty, script runs.
- [ ] One moved actor, one edited step: both reported, script does not run; after the carry and the run both still hold.
