# TutorialPolish — pending

Goal: the tutorial rooms feel staged, not abrupt: the Cat speaks before an ability page opens, pads vanish on arrival, the Shoot room starts with one centred dummy and a pause after its death, and the Special room cannot be passed without using the Special.
Priority: 2
Docs written: `AI/Tutorial/Steps.md` (steps 3, 5, "Unlock rule"), `AI/Tutorial/Flow.md` ("Rooms"), `Encounter/Action/CLAUDE.md` (`GeoShowAbilityPageAction`, `GeoUnlockAbilityAction`), `Encounter/Trigger/CLAUDE.md` (`GeoAbilityActivatedTrigger`), `AI/Python/Tutorial/CLAUDE.md`.
Read first: `AI/CodingStyle.md`, `AI/Python/Tutorial/CLAUDE.md` ("Read the scene first", "Hand edits survive a re-run"; run `encounters_diff.py` before the script).

## Done 2026-10-21 (written, not yet run)
- Code compiled (all 12 units); link blocked, the user's editor holds `UnrealEditor-GeoTrinity-Win64-DebugGame.dll`
- `GeoUnlockAbilityAction` (grant only), `GeoShowAbilityPageAction`, `GeoAbilityActivatedTrigger`, test `GeoTrinity.Encounter.AbilityActivated`
- `tutorial_encounters.py` (unlock = Cat page line + grant + page; Shoot first dummy + 2 s delay; Special `Intro` / `Fight` with `Progress`; no `Seal` step, entry never opens; stale `Tut_*_ArrivalZone` removed), `tutorial_map.py` (no arrival zone), `pie_tutorial_flow.py` (entry stays closed)

## Left (needs the editor closed)
- [x] `Tools\Build_Editor.bat`, then `Tools\Run_Tests.bat` (new test + Encounter / Tutorial suites)
- [x] `encounters_diff.py` (carry hand edits), then `tutorial_encounters.py` (creates `Tut_Shoot_FirstDummyPoint`, removes arrival zones, rewrites steps)
- [x] Headless `pie_tutorial_flow.py` (Landing → Shoot) and `pie_tutorial_jump.py`
- [ ] User plays: Deploy and Special rooms (page after the Cat line; Special sting when the beam is never pressed); tune `FIRST_DUMMY_DEATH_SECONDS` (2 s placeholder) against the death montage; rewrite the sting line
