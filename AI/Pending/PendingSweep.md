# PendingSweep — pending

Goal: go through every `AI/Pending/*.md` one by one: check whether each open row is still needed (code / assets may already do it), apply what an agent can do, delete files left with nothing an agent can do.
Docs written: none (a sweep, no feature). Scope: new features excluded (`TrainingCat`, `TutorialAggroRadius`, `MaterialFunctionDedup`, `MenuGuide`, `EncounterHandEdits`).

## Method (per file)
- Re-read the file just before acting (another session may edit `AI/Pending` at the same time).
- Check state before running anything: grep code, read assets with a headless probe (`UnrealEditor-Win64-DebugGame-Cmd.exe ... -run=pythonscript`, editor closed), compare test report time (`AI/Output/Tests/index.json`) with the DLLs before rerunning tests.
- Tutorial map scripts (`tutorial_map.py`, `tutorial_encounters.py`) can overwrite hand edits (`EncounterHandEdits.md` not done): check the map first, run them only if the row is not already true.
- An editor someone else opened holds loaded files: saves from a commandlet fail silently (`save_loaded_asset` False). Apply through its bridge (port 8090) instead, or wait for it to close.
- Animation clip scripts: `AI/Python/Anim/verify_badge_scripts.py` first (filter `TARGETS`), since a re-run wipes montage sound notifies (restored by `Boss/Anim/boss_montage_sounds.py`).

## Done (2026-10-27)
- Editor build up to date; `Tools\Run_Tests.bat GeoTrinity.` 139/139; after the Cat clips `GeoTrinity.Tutorial.` + `GeoTrinity.Cat` 39/39.
- Deleted as finished: BossLoot, CritBase, DashVoidInMap, UnequipTier, StatsPanelHidden, GemDropSounds, VariantsAsData, EnemyMontageOwnership, TutorialTestCoverage, ConeSprayTell (and earlier MinibossWaveSpots, SafeSpots, SpeechPopupBase, CatInGame).
- Applied: `WBP_CatLine` `TypeSound` = `SW_Mech_Tick_1` (class `SC_Interface`); `SCon_GemDrop` on `BP_GemDrop` spawn / pickup; `BP_ZonePattern_DamageReduction` colour `DamageReduction`; Cat face peek (`FACE_PEEK_DEGREES` in `cat_posing.py`, Reach / Flat / portal jump, `head_pitch_min` in `cat_clip.py`), every Cat clip + `ABP_Cat` re-run headless; `hex_boss_abilities.py` / `hex_boss_launch.py` reports → `AI/Output`, launch montages on `CastSlot`.

## Left
- [ ] Bomb / Turret montages: live sections `Start → Fire`, `hex_boss_launch.py` writes `Start → Fire1` (its docstring: a `Fire` section is never jumped to). Ask the user which is right; if `Fire1`, re-run `hex_boss_launch.py` then `boss_montage_sounds.py`.
- [ ] Other boss clip scripts still write their report to a dead session scratchpad (`hex_boss_death/idle/intro.py`, `star_death/devastating_wave/idle_breath/intro/pike_nova/spike_nova.py`): point `REPORT` at `AI/Output/`.
- [ ] ConeSpray tell shows 4–6 of 18 blades past the face in frames 6–30 (row asked "front blade only"); no shell punched. Tune `tell_focus` in `hex_boss_abilities.py` only if the user wants it narrower.
- [ ] Sweep the rest. Every row left needs the user (look, feel, PIE, design question) or is an excluded feature: CatAnimation, CatClipsPick, CatEvilFace, CatLocomotionIdles, CatWalkRun, CatPortrait, CatLineTypewriter, EchoSocketHalo, EchoUiPolish, SoundConcurrency (bullet impacts by ear), TutorialPolish, TutorialTeleporters, TutorialExit, TeleportPortal, GemShapes3D, GemEcho, MessageLog, QuestDummies, TestAim, Tutorial. Re-check each for a row an agent can still do; otherwise list them for the user.
- [ ] Tell the user (design, from the deleted TutorialTestCoverage): the Heal and Shield zone steps end the instant the pawn enters, so the player never feels the zone.
- [ ] Delete this file when empty.
