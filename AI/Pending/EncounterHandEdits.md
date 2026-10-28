# EncounterHandEdits — pending

Goal: re-running `tutorial_encounters.py` and `tutorial_map.py` never overwrites what the user changed by hand in the Tutorial map.
Priority: 2
Docs written: `AI/Python/Level/CLAUDE.md` ("Hand edits survive a re-run").

## Scripts
- Done by hand-sync (not by the ledger): the scene's Landing / Shoot / Deploy / Special steps, the nine Shoot respawn points and the box sizes were carried into `tutorial_encounters.py` / `tutorial_map.py`; the ledger below still protects the next hand edit.
- [ ] `AI/Python/Tutorial/Level/tutorial_map.py` — same routing through `asset_guard.write`: `dup_actor`, `spawn_volume`, `fit`, `point`, `place_wall` (rotation, scale, overlay material), `pad` (`TeleportTag`, `bOpen`, `DisplayText`), `build_pads` zones, `build_jump_start`, `build_training_start`, `world_settings`, `build_game_mode` / `set_instance_maps` defaults; `create_map` and `build_rooms` destroy / rename only actors whose transform equals the ledger (else keep + report).
- [ ] `AI/Python/Tutorial/Level/tutorial_encounters.py` — import `UI/asset_guard.py` by project-relative path; route through `asset_guard.write`: `place` (location / rotation), `fit_floor` (scale, location), `volume` (scale), `point` (`GameplayTags`), `encounter_actor` + `build_encounters` (`Steps`, `bStartOnBeginPlay`), `organize_folders` (already skips set folders); `remove_actor` only for an actor whose transform still equals the ledger; `report()` written to the report file as `kept by hand`; `asset_guard.created` for actors this run spawns.
- [ ] `Steps` compare: confirm `export_text` of the instanced array reads back stable between runs (subobject names); if not, compare a normalised dump (`Name`, rule count, action classes + properties).
- [ ] `AI/Python/UI/asset_guard.py` — comment/doc line: also used by `Tutorial/Level/tutorial_encounters.py` (ledger shared); `AI/MCP/CLAUDE.md` "Hand edits" row points to it already.
- [ ] Seed the ledger: first run records current values as the user's → nothing is overwritten the first time; ask the user which hand-changed rooms to adopt into the script.

## Verify
- [ ] Headless re-run on the current map: report lists the kept values, `git status` shows no `Tutorial.umap` change from a no-op run.
