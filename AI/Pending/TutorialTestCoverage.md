# TutorialTestCoverage — pending

Goal: the headless tutorial tests play every branch of every room, not only the main path.
Docs written: `Source/GeoTrinityEditor/Private/Tests/Tutorial/CLAUDE.md` (bot, coverage table), `AI/Testing.md` ("Tutorial runs"), `Source/GeoTrinityEditor/Public/CLAUDE.md`, `Source/GeoTrinity/Public/Encounter/CLAUDE.md` + `Action/CLAUDE.md` (`OnCatLine`), root `CLAUDE.md` (tutorial gate).

## Today's FullRun misses (why this file exists)
- Only one branch per room: no Dash falls / idle, no Shoot 45 s cut, no Special first-try / surrender / order, no Dummies death, one lane per safe-spot wave and no retry, no Miniboss wipe / tips / call-outs / wave lanes, no final exit or Leave tutorial, no jump.
- No Cat line is checked, so a wrong branch (relief vs stunned, sting vs done) passes.
- Abilities faked: Dash replaced by a teleport, Healing Zone by `HealFully`, Special by a broadcast of `AbilityActivatedCallbacks`.
- No check of pages popping, the Landing Cat hiding, quips, respawn spot, zone effects, dummy behaviour, boss points, loot.

## Code
- [x] `Source/GeoTrinity/Public/Encounter/GeoEncounterSubsystem.h/.cpp` — `FOnEncounterCatLine OnCatLine` (native multicast, `FText const&`), no Blueprint exposure.
- [x] `Source/GeoTrinity/Private/Encounter/Action/GeoCatLineAction.cpp` — broadcast `OnCatLine` with the token-replaced text when the line shows (with or without a HUD; on respawn for `bWaitForRespawn`).
- [x] `GeoTrinity.Encounter.CatLine` (`GeoEncounterTests.cpp`) — also asserts `OnCatLine` got the replaced text.
- [x] Move `Source/GeoTrinityEditor/Private/Tests/GeoTutorialRunTests.cpp` → `Tests/Tutorial/`, split per the folder's `CLAUDE.md`: `GeoTutorialBot.h/.cpp` (runner, helpers, `FGeoStartListenPlayCommand`, `FGeoRestoreGemProfileCommand`, line log, counter watch, `JumpTo`), `GeoTutorialRoomStages.cpp` (`Add<Room>Stages(Bot, Options)`), `GeoTutorialRunTests.cpp` (FullRun), `GeoTutorialPathTests.cpp`.
- [x] Bot: `bEndLines` flag (off for EarlyPortal); page log in `ServeUI` (ability tag of the page open + `IsPaused` while open); per-test stage timeout default 90 s.
- [x] FullRun additions (rows "FullRun" of the coverage table): real Dash (`AddMovementInput` toward +X for a frame, `TryActivateAbility` by `Ability.Spell.Dash` at the void edge; `Falls` stays 0, stunned line); real Healing Zone (`SpawnDeployableFromAbility` on the ally, no `HealFully`); real Special (`TryActivateAbility` `Ability.Spell.MoiraBeam`); quips 3 / 6 / 9 by line; a killed quest dummy replaced at its death spot (< 50 cm); Landing Cat `IsHidden` after the pad opens and its jump; chaser distance to the pawn drops, shooter's `Ability.Spell.ProjectileToAllPlayers` activates (mover: not built); zone effects (pawn health set to 50 % then rises in Heal; shield attribute > 0 in Shield; health drops in Damage); boss seen within 100 cm of ≥ 2 `MINIBOSS_POINTS`; profile gains the 7 chips (before restore).
- [x] Path tests (one `IMPLEMENT_SIMPLE_AUTOMATION_TEST` each, `GeoTrinity.Tutorial.Path.<Case>`, cases = coverage table): `LandingNag`, `EarlyPortal`, `DashFalls` (pawn under `KillZ` 7 times, invincibility off; respawn within 200 cm of the Dash `TargetPoint.Entrance`), `DashIdle`, `ShootCut`, `PauseHolds` (open the pause menu 2 s real time, no respawn, no cut), `SpecialFirstTry`, `SpecialOrder`, `SpecialSurrender`, `DummiesDeath`, `SafeSpotsRetry` (lanes: series 1 = −650 then out; series 2 = 650, 500, then out; series 3 = 0, −300, 250), `MinibossWipe`, `MinibossCallouts`, `MinibossWaveLanes`, `FinalExit`, `LeaveFromMenu`, `JumpCommand`.
- [x] `GeoTrinity.Tutorial.Jump.<Room>`: `IMPLEMENT_COMPLEX_AUTOMATION_TEST`, `GetTests` = Dash … Miniboss; asserts per the coverage row.
- [x] `FinalExit` / `LeaveFromMenu` / `JumpCommand`: wait for `GEditor->PlayWorld` to change map (PIE travel); `FinalExit` and the Miniboss paths restore the gem profile.
- [x] Line fragments: a `TMap` of short unique fragments per branch at the top of `GeoTutorialRoomStages.cpp`, copied from `tutorial_encounters.py` (comment: keep in step with the script).

## State (end of session)
Code written, formatted, editor target builds. Editor was closed by the agent (user said yes); nothing else of the user's touched.
- `GeoTrinity.Encounter.CatLine`: green. `GeoTrinity.Tutorial.Path.LandingNag`: green (Path infrastructure works).
- `GeoTrinity.Tutorial.FullRun`: plays Landing → Miniboss end; last run failed only on the page check (below). Before that run: Heal/Shield zone effect and the chip grant were fixed (see `Tests/Tutorial/CLAUDE.md`).
- Never run yet: other 16 `Path.*`, all `Jump.*`.

## Next steps
1. **Locked abilities are not locked in the headless PIE run.** At the pawn's first frame it already holds Dash, BasicBeam, HealingZone and MoiraBeam, though the map's `LockedAbilityTags` hold all four (checked in the saved map). Cause not found: `PlayableCharacter.cpp:178` honours the lock; `GeoCharacter.cpp:217` `GiveStartupAbilities()` (StartupAbilityTags, no lock) is the suspect: read `BP_GeoPlayableCharacter`'s ASC `StartupAbilityTags`, and whether a PIE-only path (class change, gem loadout) grants. If it is a real game bug, fix it in code (then `Page` and `Jump.*` checks hold as written); if PIE-only, bot must read "granted" differently. Until then: `AddPageCheckStage` (pages 1..4 with 1..4 abilities held) fails, and every `IsGranted` unlock stage passes vacuously; `Jump.*` ability asserts will fail for the same reason.
2. Run `Tools\Run_Tests.bat GeoTrinity.Tutorial.` (editor closed, `GEO_NO_PAUSE=1`, build first); fix each red path one by one. Untested guesses most likely to break: real Dash direction (`AddRealDashStages`), `FinalExit` / `LeaveFromMenu` / `JumpCommand` PIE server travel (`AddTravelStage`), Miniboss wipe / callout / wave-lane timing, Special `MoiraBeam` activation by `TryActivateAbilityWithTargetData`, Cat hidden after its portal jump headless (`AddLandingStages`).
3. A branch that cannot be driven headless: move to "Not played" in `Tests/Tutorial/CLAUDE.md` with the reason.
4. Observations for the user (design, not test): the Heal and Shield zone steps end the instant the pawn enters, so the player never feels the zone; the Dummies room's mover has no patrol yet.
5. Delete this file once everything is green.

## Verify
- [ ] `Tools\Build_Editor.bat` (`GEO_NO_PAUSE=1`, editor closed), full editor restart for the new tests to register.
- [ ] `Tools\Run_Tests.bat GeoTrinity.Tutorial.` — every FullRun / Path / Jump green; report counts; a branch that cannot be driven headless is moved to the "Not played" line of the folder's `CLAUDE.md` with the reason.
- [ ] `Tools\Run_Tests.bat GeoTrinity.Encounter.CatLine`.
