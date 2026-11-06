# EncounterNetwork — pending

Goal: encounters work over the network for any number of players, late joiners included, on an architecture that grows without friction: the Training Room becomes an encounter and dungeons follow. The tutorial, still solo, also plays right as a remote client.
Priority: 3
Docs written: none yet. Reflection parked at workflow step 2; approach not audited.
Size: medium refactor, ~1,000-1,500 lines of code and tests, 2-3 working sessions (rough; biggest unknown: stages the client bot breaks).

## Decided with the user
- Tutorial stays solo (`bSoloOnly`); it must also work as one remote client.
- Nothing pauses the game, ever: no solo pause either.
- Players join mid-encounter and catch up, like a late joiner in a dungeon.
- With several players, a line waits for the last player to skip it, or it is a timed line (no wait).

## Why it breaks today
Encounters run on the server only; on a dedicated server `GetFirstPlayerController()` is the remote player's controller, with no HUD. Read from the code, not run (only the first two rows were seen by the user).
| Symptom | Cause |
|---|---|
| Cat missing | `AGeoCat` not replicated: show and orders stay on the server |
| Abilities granted, not in the bar | Grant is fine. Bar cause unknown: `PostReplicatedAdd` → `OnGiveAbility` → `OnAbilitySetChanged` → rebuild looks right. The test must show it |
| Pages, quest cards, Gems page missing | `GetLocalHUD()` null on the server |
| Cat lines missing, script races | Same null HUD; no HUD = `Finish()` at once |
| Movement lock ignored | `SetIgnoreMoveInput` on the server's controller |
| Gem triggers dead | `GetFirstLocalProfile` finds nothing on a dedicated server; loot / XP path not traced |

## Architecture
Actions and triggers never know about networking or the number of players.
1. **Participants:** the players an encounter concerns; replaces every "first player". Default everyone, optional volume = players inside (Training Room: others may be elsewhere on the map). Fires an event when the list changes.
2. **Presenter:** one component on each player controller, the only network link for UI. New UI verb = one action + one RPC + one HUD call. A listen host takes the same path (its client RPCs run locally).
3. **Events vs state:** one-off events are RPCs (line, page opening). Lasting state is replicated (quest card, movement lock, Cat, pads), so a joiner or a reconnect gets it with no extra code.
4. **One wait:** the action base waits until every participant answered; a player leaving counts as answered. A joiner is not added to a wait already running. Lines and pages share it. Timed lines never wait.
5. **Catch-up on join:** participant added → triggers bind that player; lasting grants (unlocks) replay for them, same idea as `OnSkipped`; replicated state does the rest. A line already showing is not resent: the joiner sees the next one.
6. **Scope per chain, not per world:**
   - Arrival reset aborts only its own chain (`StartAfterSkippingChain` aborts every encounter of the world today: a dungeon starting would kill a Training Room encounter).
   - Movement lock per player (`UGeoEncounterSubsystem::bMovementLocked` is one world flag).
   - One shared caster per chain (one per world with one running ability per class today: two rooms cannot fire the same ability).

Open: a page has no timer, so one player leaving it open blocks the script. Add a limit with the first multiplayer encounter using pages.

## Read map
- `Source/GeoTrinity/Public/Encounter/CLAUDE.md`, `Source/GeoTrinity/Public/Encounter/Action/CLAUDE.md`, `Source/GeoTrinity/Public/Encounter/Action/Cat/CLAUDE.md`, `Source/GeoTrinity/Public/Encounter/Trigger/CLAUDE.md` — design to rewrite
- `Source/GeoTrinity/Private/Encounter/GeoEncounter.cpp`, `Source/GeoTrinity/Private/Encounter/GeoEncounterAction.cpp`, `Source/GeoTrinity/Private/Encounter/GeoEncounterSubsystem.cpp` — start gate, `GetLocalHUD`, abort, caster, lock
- `Source/GeoTrinity/Public/Encounter/Action/GeoCatSpeechAction.h`, `Source/GeoTrinity/Public/Encounter/Action/GeoQuestCardAction.h`, `Source/GeoTrinity/Public/Encounter/Action/GeoMenuGuideAction.h`, `Source/GeoTrinity/Public/Encounter/Action/GeoShowAbilityPageAction.h`, `Source/GeoTrinity/Public/Encounter/Action/GeoSetMovementLockedAction.h`, `Source/GeoTrinity/Public/Encounter/Action/GeoUnlockAbilityAction.h`, `Source/GeoTrinity/Public/Encounter/Action/Cat/GeoCatAction.h` — first-player and HUD sites (`GeoOpenGemsPageAction` no longer exists: `GeoMenuGuideAction` is the nearest; check)
- `Source/GeoTrinity/Public/Encounter/Trigger/GeoAbilityActivatedTrigger.h`, `Source/GeoTrinity/Public/Encounter/Trigger/GeoGemProfileTrigger.h`, `Source/GeoTrinity/Public/Encounter/Trigger/GeoPlayerIdleTrigger.h`, `Source/GeoTrinity/Public/Encounter/Trigger/GeoVolumeEnteredTrigger.h`, `Source/GeoTrinity/Public/Encounter/Trigger/GeoActorDiedTrigger.h` — per-player bindings
- `Source/GeoTrinity/Public/Actor/GeoCat.h`, `Source/GeoTrinity/Private/Actor/GeoCat.cpp`, `Source/GeoTrinity/Public/Actor/CLAUDE.md` ("GeoCat") — orders to replicate
- `Source/GeoTrinity/Public/Characters/CLAUDE.md` ("GeoTrainingCat"; the class is documented, no GeoTrainingCat.h in the code yet) — sibling speaker: replicated speech, check before designing the presenter
- `Source/GeoTrinity/Public/GameClasses/Player/GeoPlayerController.h`, `Source/GeoTrinity/Private/GameClasses/Player/GeoPlayerController.cpp`, `Source/GeoTrinity/Private/GameClasses/Match/GeoPlayerState.cpp` — presenter host, HUD setup, pause to remove
- `Source/GeoTrinity/Public/HUD/Interface/GeoHUDInterface.h`, `Source/GeoTrinityUI/Private/HUD/GeoHUD.cpp`, `Source/GeoTrinityUI/Private/HUD/AbilityBar/GeoAbilityBarWidget.cpp` — UI seam, bar
- `Source/GeoTrinityEditor/Private/Tests/Tutorial/` (all, with its `CLAUDE.md`) — bot to split into server and client worlds

## Before coding
- [ ] Back to workflow: `auditor` on this approach, then step 3 (`CLAUDE.md`s rewritten as if built, including the "solo host" statements in `Encounter/`, `Action/`, `Actor/`, `HUD/Interface/`, `AI/Tutorial/Flow.md`), then the user's review

## Code (to implement)
- [ ] Test bot first: client mode (PIE_Client, in-process dedicated server), separate server and client worlds (47 world / pawn / controller calls in its stages), Cat line ending on the client HUD
- [ ] Participants on `AGeoEncounter`, with change event; every first-player site moved onto them
- [ ] Presenter component on `AGeoPlayerController`; 8 actions moved onto it; `OnCatLine` stays server side for the bot
- [ ] One wait in `UGeoEncounterAction`: answers per participant, leaving = answered
- [ ] Triggers: bind each participant, react to the change event; `GeoGemProfileTrigger` through a client-to-server relay
- [ ] Late join: replay of lasting grants, movement lock applied through `ClientIgnoreMoveInput`
- [ ] `AGeoCat`: `bReplicates`, `bAlwaysRelevant` (engine: a hidden actor with no collision is not relevant); replicated order played locally by each client, each roam spot a new order; start location + time so a joiner lands mid-walk; facing and idle variants stay local
- [ ] `AGeoCat` montage on a dedicated server: blend-out may never fire and stall the script; if so, the server times it from the montage length
- [ ] Scope per chain: arrival reset, movement lock, shared caster
- [ ] Ability bar: fix once the test shows the cause
- [ ] Remove pause: `GeoPlayerController.cpp` (`SetPause`, `IsSoloOnlyWorld`), bot (`bWorldPaused`, `FPage`), `GeoTutorialRoomStages.cpp` page check, `Path.PauseHolds`, `bTriggerWhenPaused`; docs: `AI/Tutorial/Flow.md`, `Encounter/Action/`, `GameClasses/Match/`, `GameClasses/Player/`, `Tests/Tutorial/`, `HUD/Tutorial/` `CLAUDE.md`s
- [ ] Without pause the world runs behind an ability page: check each tutorial page's moment, room by room; move those in danger

## Tests
- [ ] `GeoTrinity.Tutorial.Client.*`: real map, remote client; must fail first on Cat, bar and lines; reuses the `FullRun` stages for a full client run
- [ ] Two clients on a small encounter (`GeoTrinity.Encounter.*`): a line waits for the last to skip, a timed line does not wait, a player leaving releases the wait
- [ ] Late joiner: gets unlocks, quest card, lock, Cat, and triggers fire for them
- [ ] Two encounters side by side: arrival reset of one leaves the other running, same ability fired in both
- [ ] Run: `Tools\Build_Editor.bat`, `Tools\Run_Tests.bat GeoTrinity.Encounter`, `Tools\Run_Tests.bat GeoTrinity.Tutorial.` (`test-fixer` agent)

## Verify
- [ ] Whole `GeoTrinity.Tutorial.` set green, listen and client
- [ ] User, in PIE when asked: Play as Client on the tutorial; two clients with one joining late
