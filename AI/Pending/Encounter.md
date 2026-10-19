# Encounter — pending

Goal: a generic, easy-to-author scripting system (steps → rules of trigger + condition + actions, all Blueprint-subclassable) so any scripted event, first the tutorial, is set up by hand in the level.
Docs written: `Source/GeoTrinity/Public/Encounter/CLAUDE.md`, `Trigger/CLAUDE.md`, `Action/CLAUDE.md`, row in `Source/GeoTrinity/Public/CLAUDE.md`.
Grouping checked: no existing code of this kind (boss phases live in StateTrees, not touched); nothing to move.

## Code (to implement)
- [ ] `Public/Encounter/GeoEncounterTypes.h` — `FGeoEncounterPayload`, `FGeoEncounterRule`, `FGeoEncounterStep`
- [ ] `GeoEncounterTrigger.h/.cpp` — `OnArm` / `OnDisarm` BlueprintNativeEvents, `Fire(Payload)`, `GetEncounter()`, `GetWorld()` override (Blueprint needs it)
- [ ] `GeoEncounterCondition.h/.cpp` — `Evaluate(Payload)` BlueprintNativeEvent
- [ ] `GeoEncounterAction.h/.cpp` — `Execute`, `Finish`, `OnCancel`, `bLatent`; base calls `Finish()` after `Execute` when not latent
- [ ] `GeoEncounterSubsystem.h/.cpp` — `Raise(Tag, Payload)`, multicast delegate listened to by `EventTrigger`
- [ ] `GeoEncounter.h/.cpp` — steps, rules, sequential awaited action runner, `Start`/`Restart`/`GoToStep`, `OnCompleted`, `IsDataValid`, `Geo.Encounter.Debug`
- [ ] `Trigger/`: `PlayerEnteredVolumeTrigger`, `ActorDiedTrigger`, `PlayerDiedTrigger`, `HealthThresholdTrigger`, `TimerTrigger`, `EventTrigger`
- [ ] `Action/`: `DelayAction`, `RaiseEventAction`, `StartEncounterAction`, `SetBarrierClosedAction`, `TeleportPlayersAction`
- [ ] Death hooks: find the existing delegate for player death (`AGeoGameState::NotifyPlayerDied` path) and enemy death (`AEnemyCharacter`); add a delegate on the hub that lacks one (never a call planted in the encounter)
- [ ] Read each engine header before use (`AI/CodingStyle.md`); clang-format touched files; API macros on cross-module symbols

## Config
- [ ] `Config/Tags/GeoGameplayTags.ini` — root `Encounter.Event` (children authored per use)

## Tests (headless, `AI/Testing.md`)
- [ ] `Private/Tests/GeoEncounterTests.cpp` + `Private/Tests/GeoTestEncounter.h` (fake trigger with a public fire, counting action, latent action, minimal listen-server world as in `GeoTestDeployable`)
  - `GeoTrinity.Encounter.StepFlow` — rule fires → actions in order → step completes → next step entered
  - `GeoTrinity.Encounter.Condition` — payload filtered (`Count == 2` runs the rule on the second firing only)
  - `GeoTrinity.Encounter.Repeat` — `bRepeat` false fires once and disarms; true fires every passing firing
  - `GeoTrinity.Encounter.LatentAction` — next action waits for `Finish()`; step exit calls `OnCancel`
  - `GeoTrinity.Encounter.BusyRule` — firing while the rule's actions run is ignored
  - `GeoTrinity.Encounter.StepAfter` — named jump and `GoToStep`; last step raises `OnCompleted`
  - `GeoTrinity.Encounter.Restart` — back to step 0, triggers re-armed with counters at 0
  - `GeoTrinity.Encounter.EventBus` — `Raise(Tag)` reaches `EventTrigger` with its payload
  - `GeoTrinity.Encounter.Validation` — duplicate step name, unknown `StepAfter`, null trigger flagged
- [ ] Run: `Tools\Build_Editor.bat` then `Tools\Run_Tests.bat GeoTrinity.Encounter` (`GEO_NO_PAUSE=1`, full editor restart to register new test classes)

## Verify
- [ ] Parsed report: counts and failures; no PIE unless asked
