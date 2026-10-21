# ArrivalReset — pending

Goal: taking a tutorial portal goes straight to the next encounter: the room left is cancelled, its Cat line is cut, and the world is reset to what the next room expects (abilities, locks), as the jump command does.
Docs written: `Source/GeoTrinity/Public/Encounter/CLAUDE.md` ("Arrival reset"), `Encounter/Action/CLAUDE.md` (`GeoStartEncounterAction`), `HUD/Interface/CLAUDE.md`, `GeoTrinityUI/Public/HUD/Tutorial/CLAUDE.md`, `AI/Tutorial/Flow.md` ("Rooms").

## Code (to implement)
- [ ] `Encounter/GeoEncounter.h/.cpp` — `Abort()` (stop step, no `OnExit`, `Idle`, no `OnCompleted`; through the pending-change gate when inside its own call); `StartAfterSkippingChain()` order: abort others, `HideCatLine` + `HideQuestCard`, skip replay, `Start()`
- [ ] `Encounter/Action/GeoStartEncounterAction.cpp` — `Execute` calls `Target->StartAfterSkippingChain()` instead of `Start` / `Restart`
- [ ] `HUD/Interface/GeoHUDInterface.h` — `HideCatLine()`; `GeoTrinityUI` `AGeoHUD` + `UGeoCatLineWidget` implement it (end the line, fire `OnLineEnded`)
- [ ] Test fake HUD (`Private/Tests/GeoTestEncounter.h`) — `HideCatLine` override

## Open
- [ ] Find which room's action list still shows text after its exit pad opens (a line after the pad-open action, or a rule line) — expected: `Flow.md` says the pad opens after the last line; confirm the cause before closing this file
- [ ] Cat actor on arrival (position, montage, hidden state): `OnSkipped` is a no-op for its actions; check by hand that the next room's `OnEnter` puts it right, else give it a skip override

## Tests (`Private/Tests/GeoEncounterTests.cpp`)
- [ ] `GeoTrinity.Encounter.ArrivalAbort` — start action while another encounter runs a latent action: other aborted, no `OnExit`, `Cancel` seen, target started
- [ ] `GeoTrinity.Encounter.ArrivalFromOwnRule` — the start action sits in the aborted encounter's own rule: no re-entrancy fault, target runs once
- [ ] `GeoTrinity.Encounter.ArrivalHidesLine` — fake HUD saw `HideCatLine` (and `HideQuestCard`) before the target's `OnEnter`
- [ ] `GeoTrinity.Encounter.ArrivalSkipReplay` — unlock in a skipped `OnEnter` granted when the chain was left early; lock in aborted room + replayed unlock leaves movement free
- Run: `Tools\Run_Tests.bat GeoTrinity.Encounter` after the build (editor closed, `GEO_NO_PAUSE=1`)
