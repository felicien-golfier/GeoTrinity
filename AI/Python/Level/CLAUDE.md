# AI/Python/Level

Generic level content and per-level settings. Doc: `AI/MCP/MCP_Settings.md`. Tutorial map and encounters: `Tutorial/`.

| Script | Does |
|---|---|
| `arena_walls.py` | Place arena wall and barrier states on a floor outline, each turned so local +X is its travel direction (`AI/ArenaWalls.md`) |
| `level_settings.py` | World settings, GameMode override, controller class |
| `map_guard.py` | The map gate below: `check`, `after`, `commit`, report `AI/Output/map_guard.txt`; through `AI/Python/Runtime/run_via_bridge.py` (`8090 check Tutorial`), or headless with the editor closed |

Map rules (camera volumes, outliner folders): `Tutorial/CLAUDE.md`; they apply to any script that builds a map.

## Map gate (rule)
A script never overrides fresh changes on any map. Claude checks before launching a script that writes a map; no hook, no code enforcement. Asset and widget values: `AI/UI.md`.
Map-writing scripts: `Tutorial/Level/tutorial_map.py` (Tutorial, DraftMap), `tutorial_encounters.py` (Tutorial), `training_room.py` (DraftMap), `arena_walls.py` (DraftMap), `level_settings.py` (its map); a new one joins this list.

1. `check <Map>`: saves the map if dirty, compares its file hash with `AI/Output/MapSnapshots/<Map>.hash`.
2. `SAME`, or `RESAVED` (hash changed, level did not): run the script, go to 4.
3. `CHANGED`: nothing runs, a single edit blocks the whole run. Carry each `-`/`+` line into the script (skill `sync-tutorial-scripts`), then run the script. The scene always wins, unless an unticked Pending row names that script change as still to apply. `(file matches git HEAD)` = it came with a pull or a commit: same gate, flagged once.
4. `after <Map>`: lines moved by the run. Every line from step 3 still holds, only intended lines moved. A hand change lost → fix the script, re-run.
5. `commit <Map>`: new snapshot + hash. The sync ends here, with the full check done; no throwaway-map replay.

- `NO SNAPSHOT`: sync the scripts with the scene first, then `commit`.
- Direct MCP changes count as the user's: no gate, they show up in the next diff. Before a direct MCP write, read the value; report it if strangely different.
- Snapshots and hashes are not committed.

## Snapshot format
T3D export of the level as opened (a plain asset load gives other values: construction scripts have not run), normalized so a diff shows only real changes:
- One sorted line per property: `actor label | component | property`.
- Instanced objects (`Steps` actions, triggers, conditions) inlined where referenced: their generated numbers move on every rewrite.
- Left out: actors spawned on level load (`SPAWNED_ON_LEVEL_LOAD`), engine-derived values (`DERIVED_LINE`), generated text keys. New noise in a diff → extend these two lists.
