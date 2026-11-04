# EncounterHandEdits — pending

Goal: a script never overrides fresh changes on any encounter or map (Tutorial for now, any later one too).
Priority: 2
Docs written: `AI/Python/Level/CLAUDE.md` ("Map gate", "Snapshot format").

## Read map
- `AI/Python/Level/map_guard.py` — the tool; tested headless only

## Verify
- [ ] First gate with the editor open: `check` + `after` through `AI/Python/Runtime/run_via_bridge.py`: a dirty map is saved first, no noise line against the headless snapshot (else extend `SPAWNED_ON_LEVEL_LOAD` / `DERIVED_LINE`).
