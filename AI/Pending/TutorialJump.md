# TutorialJump — pending

Goal: jump straight into any tutorial room, with everything the earlier rooms unlock already granted.
Docs written: `Encounter/CLAUDE.md` ("Jump start"), `Encounter/Action/CLAUDE.md` (`OnSkipped` on unlock / movement lock), `GameClasses/CLAUDE.md` (`OpenTutorial(Room)`), `GameClasses/Match/CLAUDE.md` ("Player starts"), `AI/Tutorial/Flow.md` ("Jump to a room"), `AI/Python/Level/CLAUDE.md`.

## Code (to implement)
- [x] `Encounter/GeoEncounterAction.h` — `OnSkipped()` `BlueprintNativeEvent`, empty default
- [x] `Encounter/GeoEncounter.h/.cpp` — `JumpStart`, `StartAfterSkippingChain()`, `FindJumpTarget()`, `FindNextInChain()`, `SkipActions()`; `BeginPlay` jump / normal start
- [x] `Action/GeoUnlockAbilityAction` — `OnSkipped`: grant without page (`GrantAbility()` shared with `Execute`)
- [x] `Action/GeoSetMovementLockedAction` — `OnSkipped` = `Execute`
- [x] `GameClasses/GeoGameInstance.h/.cpp` — `OpenTutorial(Room)`, console `Geo.Tutorial.Jump <Room>`
- [x] `GameClasses/Match/GeoGameMode.h/.cpp` — `ChoosePlayerStart`: untagged starts only without a portal (else the engine's random pick lands a normal launch on a room's jump start)
- [ ] Check at first run: the host's pawn and ASC exist when encounters' `BeginPlay` runs (the skipped unlocks need it; `GrantAbility` logs "no local player to unlock" otherwise). If not, `StartAfterSkippingChain` waits for the first local pawn and the doc gets that line

## Scripts
- [x] `AI/Python/Level/tutorial_map.py` — `build_jump_start`: `Tut_<Room>_JumpStart` `PlayerStart` on each entry pad, `PlayerStartTag` = room name
- [x] `AI/Python/Level/tutorial_encounters.py` — `JumpStart` on each encounter; `JumpStart` folder rule. Every unlock / lock is already in `on_enter` / `on_exit`
- [ ] Run both headless on `/Game/Maps/Tutorial` (editor closed): `tutorial_map.py`, then `tutorial_encounters.py`

## Verify
- [ ] Build `Tools\Build_Editor.bat` (`GEO_NO_PAUSE=1`, editor closed)
- [x] Jump by number (`0` Landing, `1` Dash…): `FindJumpTarget` walks the chain, `MovePlayerToJumpStart` puts the pawn on the room's start
- [ ] Tests headless: `GeoTrinity.Encounter` (new `JumpSkipOrder`, `JumpSkipLock`, `JumpPortal`), `GeoTrinity.Tutorial` (new `TaggedStart`)
- [ ] Headless `-game` run of `Tutorial?listen#Special` (bridge, `AI/Testing.md`): player in the Special room, Dash + BasicBeam + HealingZone granted, movement free, Special encounter running
