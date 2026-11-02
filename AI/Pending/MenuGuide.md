# MenuGuide — pending

Goal: tutorial steps 11-12 (Gems, Forge) teach the Gems menu by a guide: a dim layer over the pause menu with a hole around the one control to click next, a pulsing frame and a short hint; nothing else answers a click. Reusable for any menu path (a goal per path).
Priority: 1
Docs written: `Source/GeoTrinityUI/Public/HUD/Menu/Frame/CLAUDE.md` ("Menu guide"), `Source/GeoTrinityUI/Public/HUD/Menu/Gems/CLAUDE.md` ("Guide targets"), `Source/GeoTrinity/Public/HUD/Interface/CLAUDE.md`, `Source/GeoTrinity/Public/Encounter/Action/CLAUDE.md`, `AI/Tutorial/Steps.md` (11-12), `Source/GeoTrinityEditor/Private/Tests/Tutorial/CLAUDE.md` (coverage), `AI/Python/Tutorial/CLAUDE.md`, `Source/GeoTrinityEditor/Private/Tests/Gameplay/CLAUDE.md` (GeoTestFlags.h).

## STATE (session cut, user said "on your own", full autonomy)
Done and verified:
- All code written; Editor build green; quick suite green (all groups, incl. `GeoTrinity.MenuGuide.Scrim`, `GeoTrinity.Encounter.MenuGuide`).
- Side fix: `Tests/Gameplay/GeoTestFlags.h` (shared inline constexpr flags) because unity blobs collided once a test file was edited.
- Tutorial map rebuilt headless: `encounters_diff.py` (via a driver loading `/Game/Maps/Tutorial` first) showed only the intended Miniboss diff; `tutorial_encounters.py` ran, saved `Content/Maps/Tutorial.umap`: Miniboss room = Fight, Gems, Forge, Exit.
- `encounters_diff.py` PROPS: `bForge` -> `Goal`, `Event`.
Running / NOT yet known:
- test-fixer agent `aea0b8de7a4873d5b` (tutorial bot runs: Path.GuideStray, Path.FinalExit, FullRun, Path.MinibossWipe, Path.MinibossWaveLanes, Jump.Miniboss). Result unknown: re-run those if no report. The bot stages are brand new and may need fixes (widget names `CharacterButton`, `GemLoadoutButton`, `CraftButton`; `bHoldPauseMenu`).
Left to do:
- [ ] Tutorial runs green (above); if the agent died, rerun with test-fixer (brief: Goal, Changed = Tests/Tutorial/*, Expected = Coverage rows).
- [ ] Optional audit requested by the user: run the `auditor` agent (Code mode; Goal, Docs = this file + Frame/Gems CLAUDE.md, Changed = files below); act on real findings, then test-fixer again.
- [ ] Look of the overlay never seen (headless only): hole math is unit tested, Slate drawing and click-through are not. If the user allows, one PIE screenshot of the Gems step.
- [ ] Tick the boxes below, delete this file when empty, move/remove its row in `AI/Pending/README.md` ("Order"); fix `AI/Pending/Tutorial.md` slice 7 (already edited: guide steps ticked).
- [ ] Ask the user about a `TroobleshootingLogs.md` entry: per-file global `constexpr` test flags collide when unity blobs reshuffle (fixed by `GeoTestFlags.h`).
- [ ] The Cat lines of steps 11-12 are mine (Steps.md says "Board page 14/15"): user to review tone.

## Changed files (for the audit)
Gameplay: `Public/HUD/Interface/GeoMenuGuideGoal.h`, `GeoHUDInterface.h`, `Public+Private/Encounter/Action/GeoMenuGuideAction`; `GeoOpenGemsPageAction` deleted.
UI: `HUD/Menu/Frame/GeoMenuGuide.*`, `GeoMenuGuideWidget.*` (SConstraintCanvas), `GeoMenuPageWidget` / `GeoMenuRootWidget` (GetGuideTarget, GetShownPage), `GeoPauseMenuWidget`, `Character/GeoCharacterSheetWidget`, `Gems/GeoGem{Page,s,Loadout,Forge}Widget` (ShowPage removed), `HUD/GeoHUD.*`.
Tests: `Tests/UI/GeoMenuGuideTests.cpp`, `Encounter/GeoEncounterAbilityTests.cpp`, `Fake/GeoTestEncounter.h`, `Tutorial/GeoTutorialRoomStages.*`, `GeoTutorialPathTests.cpp`, `Gameplay/GeoTestFlags.h` + 14 files including it.
Scripts: `AI/Python/Tutorial/Level/tutorial_encounters.py` (`gem_event`, `guide`, `miniboss_steps`), `encounters_diff.py`.

## Checklist
- [x] Goal enum, HUD interface, `GeoMenuGuideAction` (old action removed)
- [x] Page/root virtuals, overlay widget, Gems/Loadout/Forge/Character targets, `AGeoHUD`
- [x] `tutorial_encounters.py` steps, map rebuilt
- [x] Tests `GeoTrinity.MenuGuide.Scrim`, `GeoTrinity.Encounter.MenuGuide` green; quick suite green
- [x] Bot: FullRun, Path.GuideStray, Path.FinalExit, Path.MinibossWipe, Path.MinibossWaveLanes, Jump.Miniboss green
- [ ] Audit done, findings handled

## STATE 2 (after audit; user cleans this file)
Done this session, UNBUILT until the test-fixer in flight reports (agent `a29d8d778a30723ae`: Editor build, headless Tutorial map rebuild, quick suite, 6 bot runs; if it is gone, rerun them with test-fixer):
- Bot runs green before the audit fixes (FullRun, GuideStray, FinalExit, MinibossWipe, MinibossWaveLanes, Jump.Miniboss); fix was in `GeoTutorialRoomStages.cpp` (close the held menu before clearing `bHoldPauseMenu`, two stages).
- Audit fixes applied (code): scrim hole = exact control rect, `HolePadding` only pads outline + hint (`GeoMenuGuideWidget.cpp`); frame timer sleeps when no goal (`IsGuiding`, `Wake`); Loadout `GetGuideTarget`: picked-chip branch only with target sockets, tier-filter (ALL tab) before unequip; test flags in `GeoEncounterAbilityTests.cpp`; docs (Frame, Gems, Tests/Tutorial CLAUDE.md) fixed; the "socket picked first" coverage claim dropped (no such bot stage).
- `TroobleshootingLogs.md`: unity-blob entry added. Cat lines: Forge now "Look, just enough shards for one chip. Total coincidence. Absolutely not planned." + "Open the Forge and make your first chip. I'll wait. It's mandatory. I'm a cat of many mandatory things."; crafted: craftspurrson line, "Crafted. You've mastered consumerism.", exit line (script `miniboss_steps`; Gems lines unchanged). The "tired cynic" Cat trait was tried and reverted by the user (Cat.md / Tone.md back to HEAD).
Open:
- [ ] Wait for / rerun test-fixer; if the map was rebuilt before the consumerism line was re-added, rebuild again (diff first, `AI/Python/Tutorial/CLAUDE.md`).
- [ ] Audit finding 1 (unverified): `SwallowClick` returns plain `Handled()`; a click on the scrim may move focus to the viewport. Check in PIE, fix only if confirmed.
- [ ] Audit finding 5 (decision): keyboard Enter / gamepad A / d-pad / Backspace-B are not gated while the guide shows; doc says only the mouse is. Gate them, or keep and say so.
- [ ] Audit finding 9: no headless test of `FindTarget` / Loadout / Forge `GetGuideTarget` branches (filter, no free chip, socket-first, picked chip without socket); add one (and a socket-first bot stage if wanted).
- [ ] Overlay look never seen (one PIE screenshot of the Gems step if the user allows).
- [ ] Per-file task: unify Cat lines with the Tutorial Board artifact (page 14/15) if wanted; Steps.md still says "Board page 14/15".
- [ ] When all above is done: delete this file, drop its row in `AI/Pending/README.md` "Order".
