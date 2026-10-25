# DashVoidInMap — pending

Goal: the Dash room's void is always there: it is part of the map itself, not cut later by the encounters script (`tutorial_map.py` refits the Dash floor to full width on every run, which closes the hole whenever it runs after `tutorial_encounters.py`).
Docs written: `AI/Tutorial/Steps.md` (step 2 level rule), `AI/Python/Level/CLAUDE.md` (script rows).

## Scripts
- [x] `tutorial_map.py` builds near floor, far floor, `Tut_Dash_PlatformVolume` (`build_dash_void`); `tutorial_encounters.py` no longer builds floors and raises when the volume is missing
- [ ] Run, headless, editor closed: `tutorial_map.py` then `tutorial_encounters.py` (unbuilt, unrun: small change)

## Verify
- [ ] Reports are OK; re-running either script in either order leaves the 300 cm hole
- [ ] User PIE check: the 300 cm gap is crossable by the dash (headless world cannot run it, see `Pending/Tutorial.md` slice 3)

## Status 2026-10-25
`build_dash_void` and `Tut_Dash_PlatformVolume` exist. Remaining: headless runs of `tutorial_map.py` / `tutorial_encounters.py`, re-run order check, user PIE dash check. Doc path is `AI/Python/Tutorial/CLAUDE.md`, not `Level/`.
