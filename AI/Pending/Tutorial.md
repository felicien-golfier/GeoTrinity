# Tutorial — pending

Goal: a new player learns move, dash, shoot, deploy, Special, zone colours, lethal waves, gems and the Forge in a solo tutorial narrated by a sarcastic geometric Cat (MMO parody, tabletop-RPG lore), then lands in the Training Room.
Docs written: `AI/Tutorial/CLAUDE.md`, `Tone.md`, `Lore.md`, `Cat.md`, `IntroBoard.md`, `Steps.md`, `Flow.md`; `Settings/CLAUDE.md` (`bTutorialPromptAnswered`), `System/CLAUDE.md` (Session), `GeoTrinityUI/.../HUD/Menu/CLAUDE.md` ("Tutorial entry"), `Characters/CLAUDE.md` ("HiddenCasterCharacter"), `AI/Python/Level/CLAUDE.md` (`training_room.py`); the encounter system it runs on (`Encounter/CLAUDE.md`) is built.
Depends on: `HiddenCaster.md` (slice 5), `BossLoot.md` (slices 6-7), `MenuGuide.md` (slice 7, later). `VoidDeath.md` done.

Gate: each slice starts by writing the `CLAUDE.md` rows listed under its "Docs first", then stops for review if the design moved. Lasting rules live in `AI/Tutorial/*.md`; this file holds only what is left to do.

## Open design (user)
- [ ] User edits text in the artifacts; Claude reads them back and syncs the docs
- [ ] User reviews pages 4-10 text (key labels on the board are placeholders; in game they come from the HUD key-label system)
- [ ] Page 5: user validates the text; 3rd+ fall bank ≥ 4 lines; idle tier delays (seconds)
- [ ] Page 6: user validates the quest-gag text and "Greg"
- [ ] Page 13: user writes the ally low-health line ("I need healing" + Genji) and the three death tips
- [ ] New Board page "Prompt" (the Cat says you SHOULD; refusing = "incompetent and ignorant"); then sync `Flow.md` / `Cat.md` if it adds rules
- [ ] User sends further steps, if any → pages + `Steps.md` rows
- [ ] Cat palette: exact boss colour slot and variations (portrait uses a placeholder, `CatPortrait.md`; hologram slice 8)
- [ ] Training Room map design (class select, dummies, zones, teleport to lobby; no boss portals): after the tutorial ships, own `CLAUDE.md` + Pending file

## Slice 2b — Audit fixes (2026-10-18; docs written, code waits for the user's go)
Docs written: `AI/Tutorial/Steps.md` (step 2 shake, step 3 cut, hurt ally, steps 7-9 shape, step 10 call-out, steps 11-12 guide, Tuning, unlock line), `Flow.md` ("Menus pause"), `GameClasses/Player/CLAUDE.md` (solo pause, `InteractUIAction`), `GameClasses/Match/CLAUDE.md`, `GeoTrinityUI/.../HUD/Tutorial/CLAUDE.md` (Cat line above menus, skip, quest card on world time), `Encounter/Trigger/CLAUDE.md` (`WatchedClass` on the health trigger), `Encounter/Action/CLAUDE.md` and `Characters/Component/CLAUDE.md` (rows removed).
- [x] `GameClasses/Player/GeoPlayerController.h/.cpp` — `OpenPauseMenu` / `ClosePauseMenu`: `SetPause(true/false)` when the world's `AGeoGameMode` has `bSoloOnly` (null game mode on a client: no pause); check `IA_ToggleMenu` and the menu's other actions have `bTriggerWhenPaused` (`MenuInputMapping`), else Esc cannot close a paused menu — code done; the `bTriggerWhenPaused` check is an editor item
- [x] `GeoPlayerController` — `InteractUIAction` (`TObjectPtr<UInputAction>`, `Category = "GeoInput"`), bound for local player 0 next to `ToggleMenuAction`, broadcasts native `FOnInteractUI OnInteractUI`
- [x] (done via bridge, check saved) Editor: `IA_InteractUI` in `InputMapping` (`IMC_Default`), default key `F` + `Gamepad_FaceButton_Top` (user: confirm both are free); listed in the key bindings page; `BP_GeoPlayerController` `InteractUIAction` set
- [x] `HUD/Tutorial/GeoCatLineWidget` — drop `SkipKey` and the mouse polling (`WasSkipPressed`); bind the owning controller's `OnInteractUI` in `NativeConstruct`, unbind in `NativeDestruct`; skip only while a line shows and past `SkipGraceSeconds`
- [x] `AGeoHUD` — Cat line added at `CatLineZOrder` (default 110, above `PauseMenuZOrder` 100), the quest card keeps `TutorialWidgetZOrder`
- [x] `HUD/Tutorial/GeoQuestCardWidget` — countdown from `GetWorld()->GetTimeSeconds()` (`ShownTime` = world time at `ShowCard`), still on its Slate active timer
- [x] `Encounter/Trigger/GeoHealthThresholdTrigger.h/.cpp` — `WatchedClass` (`TSubclassOf<AGeoCharacter>`) besides `Watched`; per-actor binding on the native Health / MaxHealth change delegates with the actor as payload, per-actor side state (`TMap<TWeakObjectPtr<AActor>, bool>`); `UWorld::AddOnActorSpawnedHandler` while armed for the class; MaxHealth 0 = on no side; everything unbound in `OnDisarm`
- [x] Remove `Characters/Component/GeoLowHealthEventComponent.h/.cpp` whole, its use on `BP_TutorialMinibossAlly`, and the tag `Encounter.Event.Tutorial.AllyLowHealth` (`GeoGameplayTags.ini`) — code and tag done; the component on `BP_TutorialMinibossAlly` is removed in the editor (it does not exist yet: slice 6)
- [x] Remove `Encounter/Action/GeoSetHealthRatioAction.h/.cpp` and `GeoTrinity.Encounter.SetHealthRatio`; `tutorial_encounters.py`: drop the `Setup` steps of steps 4-5 and the action (line ~201) — done (script and action); the placed allies still point at `BP_TutorialHurtAlly` (`ALLY_BP`), which the editor step below creates first
- [x] (done via bridge: GE Health 50, BP_TutorialHurtAlly, allies re-placed by tutorial_encounters.py, CT rows dropped) Editor: `GE_TutorialHurtAllyAttributes` (copy of `BP_TrainingAlly`'s `DefaultAttributes` effect, Health = 0.1 × MaxHealth); `BP_TutorialHurtAlly` (child of `BP_TrainingAlly`, `DefaultAttributes` = it); the placed allies of steps 4-5 become `BP_TutorialHurtAlly`; `CT_Tutorial`: drop rows `AllyDummyStartHealthRatio`, `AllyDummyLowHealthRatio` (`tutorial_assets.py`)
- [x] clang-format every touched file
- [x] Tests: `GeoTrinity.Encounter.HealthThresholdClass` (`GeoEncounterTests.cpp`) — an actor of `WatchedClass` spawned after arming fires `Below` with `Actor` = it; one of another class never; a spawn at MaxHealth 0 does not fire; `GeoTrinity.Encounter.HealthThreshold` still passes. Solo pause, Cat line skip and Z-order: no headless layer (need a local player and viewport); checked in PIE by the user when asked
- [x] Run: `Tools\Build_Editor.bat` OK; `Tools\Run_Tests.bat GeoTrinity` 62/62, `GeoTrinityUI` 1/1

## Slice 0 — Training kit rename
Arena tag and every `Tutorial`-named asset of the Training Room become `Training*` (`Flow.md`, "Maps"), freeing `Tutorial` for the tutorial map.
- [x] `Config/Tags/GeoGameplayTags.ini`: `Arena.Tutorial` → `Arena.TrainingRoom`; `Ability.Spell.Tutorial*` (6) → `Ability.Spell.Training*`; `TargetPoint.Tutorial.*` (6) → `TargetPoint.Training.*`; DevComments say "Training Room"; one `GameplayTagRedirects` line each (precedent `Arena.Main` → `Arena.Star`)
- [x] Assets, renamed in the editor so references follow (`unreal.EditorAssetLibrary.rename_asset`, `AI/MCP/CLAUDE.md`), then fix up redirectors:
  - folders `/Game/AbilitySystem/Abilities/Enemy/Tutorial`, `/Game/Characters/Enemies/Tutorial`, `/Game/AI/Tutorial`, `/Game/Actors/Tutorial` → `.../Training`
  - `GA_Tutorial_*` → `GA_Training_*`; `BP_TutorialZoneCaster` → `BP_TrainingZoneCaster`; `BP_TutorialAlly` → `BP_TrainingAlly`; `BP_TutorialTotemBullets` → `BP_TrainingShooter`; `ST_Tutorial_*` → `ST_Training_*`; `BP_ZonePattern_*`, `BP_Zone_*`, `BP_Dummy` keep their names
  - `/Game/Characters/Enemies/BP_Dummy` is a redirector to `Tutorial/BP_Dummy`: fixed up with the rest
- [x] Re-save `DraftMap` and every asset holding the old tags (gameplay ability catalog, the `GA_Training_*`), then delete the 13 redirect lines: `Arena.Tutorial` must be free for slice 3's room tags
- [x] `AI/Python/Level/tutorial_room.py` → `training_room.py`: tag, folder and asset constants; `GA_PARENT` is stale (`GeoZoneAbility` removed in `45c4c64d`; the `GA_` now derive from `PatternAbility` with `BP_ZonePattern_*`): make `build_assets()` build that shape, or drop the stage if the assets are hand-kept now (read the script first); docstring says Training Room
- [x] Verify: bridge read of `DraftMap` (`AI/Testing.md`): no actor or asset still carrying a `*.Tutorial*` tag, `AGeoDummyArena` of the room has `ArenaTag` = `Arena.TrainingRoom`

## State at stop (next session starts here)
- 2026-10-18 end: Slice 2b code built + tests green; its editor items done through the bridge (editor was left open, launched with `-RemoteControlAllow`). NOT confirmed: `save_dirty_assets.py` result and `Runtime/pie_tutorial_flow.py` re-run (the call was cut): re-run both, then read `AI/Output/pie_tutorial_flow.txt`. Also unchecked: `IA_InteractUI` mapped keys on `BP_GeoInputMapping` (F, Gamepad_FaceButton_Top), `IA_ToggleMenu` `trigger_when_paused` true, `BP_GeoPlayerController.InteractUIAction` set.
- Next: slice 4 step 6 (mover patrol), then slices 5-8 once `HiddenCaster.md` / `BossLoot.md` / `MenuGuide.md` land.
- Menu WBPs compile again (`tutorial_menu.py`). Tutorial map: 8 contiguous rooms (`tutorial_map.py`), encounters for Landing, Dash, Shoot, Deploy, Special, Dummies (`tutorial_encounters.py`); rooms are sealed, the teleporter pads are the only way on; the player starts in Landing.
- `AbilityBar` follows grants through `UGeoAbilitySystemComponent::OnAbilitySetChanged` (no tick).
- `BP_TrainingChaser` + `ST_Training_Chase` (Root → Chase, no fight gate) built by `training_room.py`; its bullets / zones creation paths still assume the old `ST_EnemyBehaviour` states (only run when those assets are missing).
- `HiddenCaster.md`: `BP_TrainingZoneCaster` reparented to `AHiddenCasterCharacter` (tick it there).

## Slice 1 — Shell (menu in, leave out)
Docs first: `GameClasses/Match/CLAUDE.md` (`bSoloOnly` rule), `GameClasses/CLAUDE.md` (the two map refs on `UGeoGameInstance`).
- [x] `GameClasses/GeoGameInstance.h` — `TutorialMap`, `TrainingRoomMap` (`TSoftObjectPtr<UWorld>`, `Category = "GeoOnline"`, next to `MainMenuMap`)
- [x] `System/GeoSessionSubsystem.h/.cpp` — `HostListen(MapPackagePath, Portal = FString())`: URL `<Map>?listen`, `#Portal` appended when not empty; built by a static `MakeListenUrl(Map, Portal)` (tested); `GeoLocalConnectWidget` caller unchanged
- [x] `GameClasses/Match/GeoGameMode.h/.cpp` — `bSoloOnly` (`EditDefaultsOnly`, false); `InitNewPlayer` override returns an error when `bSoloOnly` and another player controller is already in the world (`GetNumPlayers` counts only players past `PostLogin`). No new class: `BP_TutorialGameMode` is a child of `BP_GeoGameMode` and inherits its defaults
- [x] `Settings/GeoGameUserSettings.h` — `UPROPERTY(Config) bool bTutorialPromptAnswered`
- [x] `GeoTrinityUI/.../HUD/Menu/GeoTutorialPromptWidget.h/.cpp` — Cat text, `DoItButton` / `SkipButton`; both set the flag + `SaveSettings()`; do it = `HostListen(TutorialMap)`
- [x] `GeoMainMenuWidget` — `TutorialButton` (always there) → `HostListen(TutorialMap)`; shows the prompt on construct when the flag is false
- [x] `GeoPauseMenuWidget` — `LeaveTutorialButton`, visible only when the world's package is `TutorialMap`'s; → `HostListen(TrainingRoomMap, "TrainingRoom")`
- [x] Editor: map `/Game/Maps/Tutorial`, `BP_TutorialGameMode`, `BP_GeoGameInstance` map refs, `DraftMap` `PlayerStart_TrainingRoom` (`tutorial_map.py`)
- [x] Editor: `WBP_TutorialPrompt`, `TutorialButton`, `LeaveTutorialButton` (`tutorial_menu.py`); prompt text is a placeholder until Board page "Prompt" is validated
- [x] Tests `Private/Tests/GeoTutorialTests.cpp` (listen-server world, `GeoTestWorld.h`):
  - `GeoTrinity.Tutorial.SoloOnly` — `bSoloOnly`: first player accepted, a second refused with an error; off: both accepted
  - `GeoTrinity.Session.ListenUrl` — `MakeListenUrl` with and without a portal
- [x] Run: `Tools\Build_Editor.bat`, then `Tools\Run_Tests.bat GeoTrinity.Tutorial` and `GeoTrinity.Session` (`GEO_NO_PAUSE=1`, editor closed)
- [x] After the travel the host spawns at the `TrainingRoom` start: checked in a `-game` process, (-6000, 0) (`Runtime/pie_leave_tutorial.py`)

## Slice 2 — Cat line and ability page seam
Docs first: `HUD/Interface/CLAUDE.md` (two calls below), new `GeoTrinityUI/.../HUD/Tutorial/CLAUDE.md` (Cat line widget, markup, quest card of slice 4), `HUD/Menu/Character/CLAUDE.md` (open on an ability), `Encounter/Action/CLAUDE.md` (`GeoCatLineAction` row).
- [x] `HUD/Interface/GeoHUDInterface.h` — `ShowCatLine(FText Line)`, `ShowAbilityPage(FGameplayTag AbilityTag)`; `AGeoHUD` implements them
- [x] `GeoTrinityUI/.../HUD/Tutorial/GeoCatLineMarkup.h/.cpp` — pure markup → rich text; key resolver moved to `UHudFunctionLibrary::GetMappedKey`
- [x] `GeoTrinityUI/.../HUD/Tutorial/GeoCatLineWidget.h/.cpp`, `GeoShakeTextDecorator.h/.cpp` (C++ done)
- [x] Editor: `WBP_CatLine`, `DT_CatLine`, `BP_GeoHudMain` `CatLineWidgetClass` (`tutorial_hud.py`); look not reviewed in game
- [x] `UGeoCharacterSheetWidget` — open on an ability: pause menu → Character sheet page → that ability's `GeoAbilityDetailWidget` drawer; `AGeoHUD::ShowAbilityPage` drives it
- [x] `Encounter/Action/GeoCatLineAction.h/.cpp` — `Line` (`FText`), `bWaitForRespawn` (a line fired by a death shows once the player is alive again); non-latent; calls `ShowCatLine` on the first local player's HUD (same process: solo listen host, `Flow.md`)
- [x] Tests `GeoTrinityUI/Private/Tests/GeoCatLineTests.cpp`: `GeoTrinityUI.Tutorial.CatLineMarkup` — bold, shake, key token, unknown token left as text
- [x] Run: build, then `Tools\Run_Tests.bat GeoTrinityUI.Tutorial`

## Slice 3 — Steps 0-2 (Landing, Move, Dash)
Needs `VoidDeath.md` done. Docs first: `Encounter/Trigger/CLAUDE.md` (`GeoPlayerIdleTrigger` row).
- [x] `Encounter/Trigger/GeoPlayerIdleTrigger.h/.cpp` — fires each `Delay` s while the player moved less than `Radius` since the last firing or arm; `Count` = idle tier (step 1 nag, step 2 idle tiers); reset by a respawn
- [x] `Config/Tags/GeoGameplayTags.ini` — `Arena.Tutorial.<Room>` per room (`Landing`, `Move`, `Dash`, `Shoot`, `Deploy`, `Special`, `Dummies`, `Zones`, `Miniboss`)
- [x] Editor, tutorial map: per room an `AGeoArenaVolume` + `TargetPoint.Entrance` points for its tag; rooms linked by `AGeoArenaBarrier`s opened with `GeoSetBarrierClosedAction`; move marker (step 1); void line (a real hole, between a walk-off and 500 cm wide) + far platform (step 2) — built by `tutorial_encounters.py` (void 300 cm, near floor 600 cm, far platform)
- [x] Editor: one `AGeoEncounter` per room; step 2 = `Falls` (`GeoActorDiedTrigger`, `WatchedClass` = player, `bRepeat`, Cat line by `Count` 1 / 2 / ≥ 3 + `GeoAddToCounterAction Falls`), reach (`GeoVolumeEnteredTrigger` on the platform, `GeoCompareCondition` `Falls == 0` → stunned line, else dry relief), idle tiers (`GeoPlayerIdleTrigger`) — built by `tutorial_encounters.py`; checked by `Runtime/pie_tutorial_flow.py` (Landing → Move → Dash no-fall → Shoot, doors open in order, no encounter warning)
- [x] Tests `GeoEncounterTests.cpp`: `GeoTrinity.Encounter.PlayerIdle` — fires per tier while still, not while moving, restarts after a move
- [ ] Tests `GeoVoidDeathTests.cpp`: `GeoTrinity.Characters.DashOverVoid` — a dash from one floor edge over a gap narrower than 500 cm lands alive on the far floor (root motion over a real hole; if the headless world cannot run the dash, say so and ask the user for a PIE check) — NOT WRITTEN: the headless test world does not tick actors, and ticking the movement component by hand leaves the character hovering in Walking mode over the gap (no floor, no fall); needs the PIE check by the user
- [x] Run: build, then `Tools\Run_Tests.bat GeoTrinity.Encounter` and `GeoTrinity.Characters` (`GeoTrinity.Encounter` 24/24 incl. `SetHealthRatio`; `Characters` 3/3)

## Slice 3b — Landing + Move merged (2026-10-19, built; one Board-text item left)
Goal: steps 0 and 1 are one room and one encounter; movement is locked while the Cat talks; the exit pad is the Move target and opens after the last Move line; the "reached a portal" line moves to the Dash room, where Dash is granted on arrival.
Docs written: `Tutorial/Steps.md` (steps 0-2), `Tutorial/CLAUDE.md` (invariant), `Tutorial/Flow.md` ("Rooms"), `Encounter/Action/CLAUDE.md` (`GeoSetMovementLockedAction`), `AI/Python/Level/CLAUDE.md`.
- [x] `Encounter/Action/GeoSetMovementLockedAction.h/.cpp` — `bLocked`; first local player's controller `SetIgnoreMoveInput(bLocked)` (counted); the lock lives on the controller, which a respawn does not reset, and the Landing room has no void, so no `OnRevived` re-apply
- [x] Dash locked at start: add the Dash ability tag to the tutorial map's `LockedAbilityTags` (editor, `World/CLAUDE.md`); the Dash encounter's first `OnEnter` = the "reached a portal" line, then `GeoUnlockAbilityAction` (Dash) — check the ability page popping on arrival suits the whistle silence that follows, and that the dash is a class ability `GeoUnlockAbilityAction` can grant
- [x] Tests (`GeoEncounterTests.cpp`): `GeoTrinity.Encounter.SetMovementLocked` — lock sets the flag, unlock clears it, a double lock needs one unlock
- [x] `tutorial_map.py` — drop the `Move` room (8 rooms: Landing, Dash, Shoot, Deploy, Special, Dummies, Zones, Miniboss; Landing keeps the Move room's size if the pad needs space), its pads / arrival zone; remove tag `Arena.Tutorial.Move` (`GeoGameplayTags.ini`); delete the old `Tut_Move_*` actors from `/Game/Maps/Tutorial`; Landing gets an exit pad at the old marker spot (`Tut_Move_Marker` gone, no marker volume)
- [x] `tutorial_encounters.py` — `landing_steps` + `move_steps` become one encounter in the Landing room: `OnEnter` = lock, Landing lines, Move lines (the "Lesson one" Landing line and the first Move line overlap: re-read the Board's page 3-4 text and keep what the user wrote), then open the exit pad + unlock; `Move` idle rule armed after the unlock; entering the open pad is the exit (no completes rule of its own); the Dash encounter's first line = "You reached a portal by walking. Truly the peak of the genre. Next lesson: gravity.", before the whistle silence; `ROOMS`, `ROOM_ORDER` and the `Landing` / `Move` dict entries updated
- [ ] Sync the "Dungeon & Dihedral" situation line into `landing_steps` once it is on the published Board's page 3 (the published Board, read 2026-10-19, does not have it yet); Board page 4 beat 3 still says "square" where the Dash room's arrival line says "portal" (`Steps.md`): fix the Board text
- [x] Verify: build OK, `GeoTrinity.Encounter` 29/29; `Runtime/pie_tutorial_flow.py` adapted and PASSED headless (lock on at start, off when the pad opens; Landing → Dash → Shoot; the Dash unlock page popped on arrival)

## Slice 4 — Steps 3-6 (unlock, quest gag, dummies, allies)
Docs first: `World/CLAUDE.md` (`LockedAbilityTags`), `AbilitySystem/Components/CLAUDE.md` (startup grant skips them), `Encounter/Action/CLAUDE.md` (`GeoUnlockAbilityAction`, `GeoSpawnEnemiesAction`), `HUD/AbilityBar/CLAUDE.md` (slots rebuilt on a grant), `HUD/Tutorial/CLAUDE.md` (quest card).
- [x] `World/GeoWorldSettings.h` — `LockedAbilityTags` (`FGameplayTagContainer`): class abilities not granted at startup in this map (tutorial map: the Circle's basic attack, deployable, Special)
- [x] `GeoAbilitySystemComponent::GiveStartupAbilities(EPlayerClass)` — skips abilities whose `AbilityTag` is in the world's `LockedAbilityTags`
- [x] `Encounter/Action/GeoUnlockAbilityAction.h/.cpp` — `AbilityTag`; grants the player's class ability with that tag (`UAbilityInfo` entry, its `InputTag`) through `GiveAbilitySpec`, once; then `ShowAbilityPage(AbilityTag)`
- [x] `GeoAbilityBarWidget` — rebuilds on `UGeoAbilitySystemComponent::OnAbilitySetChanged` (replaced the per-tick diff, passives included); tested in `UnlockAbility`
- [x] `Encounter/Action/GeoSpawnEnemiesAction.h/.cpp` — `EnemyClass`, `Count`, `RespawnDelay` (0 = none), spawn points (`TargetPoint` purpose + arena tag); latent until cancelled; `bKillOnStepExit` kills every one it spawned (the step 3 "burst")
- [x] `GeoTrinityUI/.../HUD/Tutorial/GeoQuestCardWidget.h/.cpp`, interface calls, `Encounter/Action/GeoQuestCardAction` (C++ done)
- [x] Editor: `WBP_QuestCard`, `BP_GeoHudMain` `QuestCardWidgetClass` (`tutorial_hud.py`); look not reviewed in game
- [x] Editor: step 3 = two encounter steps (first kill → card + spawner; quips at `Count` 3 / 6 / 9; cut at the first of `Count == 12`, `GeoTimerTrigger` 45 s → counter 100/100, cancel the spawner); dummies harmless (`BP_Dummy`) — `tutorial_encounters.py`; the cut ran on the 45 s timer in the flow check, the 12-kill path is not exercised
- [x] Editor: steps 4 and 5 each their own room and placed ally (now `BP_TutorialHurtAlly`, slice 2b); step 5 done = `GeoHealthThresholdTrigger Above 1.0` on its ally + `GeoActorDiedTrigger` on the enemy, both `bRepeat` false, each `+1` to `Goals`, `GeoCounterTrigger Goals == 2` completes — rooms, placed allies, `GeoSetHealthRatioAction` (new: nothing set an ally's starting health) in a `Setup` step before the heal / fight step
- [ ] (reshaped by `EncounterSpawnSetup.md`: the `BP_Tutorial*` copies go, setups on the spawn action instead) Step 6 dummies: chaser = new `BP_TrainingChaser` + `ST_Training_Chase` (`STTask_ChaseTarget`); shooter = `BP_TrainingShooter`; mover = new `BP_TrainingMover` (patrol, harmless) — chaser and shooter done (`BP_TutorialChaser`, `BP_TutorialShooter` + `ST_Tutorial_Shooter`, no arena gate); mover is a standing dummy (`BP_TutorialMover`): patrol still to do (the map has no navmesh); the tutorial copies die (`ResetToFullLifeWhenReachingZero` off)
- [ ] (wiring done by `EncounterSpawnSetup.md`: `Setup.MaxHealth` reads the rows) `CT_Tutorial` (curve table beside `CT_BossHex`): rows of `Steps.md` "Tuning", placeholder values, `MinibossHealth` = 500; dummy and ally health read it — `tutorial_assets.py`; rows exist, NOT yet wired to dummy / ally health (their `DefaultAttributes` effect)
- [x] Tests: `GeoTrinity.Tutorial.LockedAbilities`, `GeoTrinity.Tutorial.UnlockAbility` (on `GiveClassAbility`: the action itself needs a playable character on a PlayerState), `GeoTrinity.Encounter.SpawnEnemies`
- [x] Run: build, then `Tools\Run_Tests.bat GeoTrinity.Tutorial` and `GeoTrinity.Encounter`

## Slice 5 — Steps 7-9 (zone colours, lethal wave, safe spots)
Needs `HiddenCaster.md` done.
- [ ] Editor: zones room = an `AGeoDummyArena` whose boss is a `BP_HiddenCaster_TutorialWaves` child listing only the new lateral-wave abilities with safe-spot holes (existing hazard patterns, `Abilities/Pattern/CLAUDE.md`); `TargetPoint.Training.*` points tagged `Arena.Tutorial.Zones`; each fired by `GeoActivateAbilityAction`
- [ ] Editor: step 7 = steps `Zones_Heal`, `Zones_Shield`, `Zones_Damage`: `OnEnter` = `GeoSpawnZoneAction` (heal / damage reduction / damage over time effects inline, colour Heal / Shield / Damage) on a `TargetPoint.ZoneSpawn` point tagged `Arena.Tutorial.Zones`; rule `GeoVolumeEnteredTrigger` on a box placed over that point (box = zone size), `bCompletesStep` (`EncounterSpawnSetup.md`)
- [ ] Editor: step 8 = `GeoActorDiedTrigger` (player) completes, `StepAfter` = `SafeSpotsWait`
- [ ] Editor: step 9 = `SafeSpots`: `OnEnter` = one `GeoActivateAbilityAction` per wave (`bWaitForEnd` true, spots move and shrink wave to wave), then `GeoAddToCounterAction WavesDone +1`; rule `GeoCounterTrigger WavesDone == 1` completes (next = step 10); rule `GeoActorDiedTrigger` (player) → `GeoCatLineAction` (`bWaitForRespawn`), `bCompletesStep`, `StepAfter` = `SafeSpotsWait`. `SafeSpotsWait`: rule `GeoTimerTrigger` `Delay` = `DeathTime`, `bCompletesStep`, `StepAfter` = `SafeSpots`
- [ ] Tests: none new (stock triggers / actions already tested); the room is checked by the user in PIE when asked

## Slice 6 — Step 10 (miniboss)
Needs `BossLoot.md` done. Docs first: `Actor/Arena/CLAUDE.md` (`SpawnAdd`, `bRecordsLeaderboard`), `Characters/CLAUDE.md` (`GeoSummonedDummy` row + rules), `Abilities/Boss/CLAUDE.md` (`GeoSummonAddAbility` row).
- [x] `Actor/Arena/GeoArena.h/.cpp` — `SpawnAdd(TSubclassOf<AEnemyCharacter>, FTransform)` (server): spawns and registers in `Adds`, so a wipe (`ResetBoss` → `ResetAdds`) and the boss's death remove summoned dummies and allies; `bRecordsLeaderboard` (default true; tutorial miniboss false) gates `UGeoLeaderboardSave::Record`
- [x] `AbilitySystem/Abilities/Boss/GeoSummonAddAbility.h/.cpp` — telegraphs a spot, lands the hit (the pillar's damage zone, `Actor/Deployable`, Damage colour), then `Arena->SpawnAdd(AddClass)` there
- [x] `Characters/GeoSummonedDummy.h/.cpp` — `AEnemyCharacter` child: a zone at its feet growing over `DummyZoneGrowthTime` to `DummyZoneMaxRadius`, ticking `DummyZoneDamage`, destroyed with it; on death `Arena->SpawnAdd(AllyClass)` at its place (healable, a bullet hit target so it covers, shoots the boss; now `BP_Dummy` + `AllySetup`, `EncounterSpawnSetup.md`)
- [ ] Miniboss `BP_TutorialMiniboss` + `ST_TutorialMiniboss`: moves between 3 level `TargetPoint`s, fires projectiles, summons; `STTask_SendEventAfterNCycles` after each summon (`CyclesRequired` = N) sends the lateral-wave event; health from `CT_Tutorial` `MinibossHealth`
- [ ] Editor: miniboss room `AGeoArena` (`BossLoot`: override on, count 0, one of each Chip, `FixedShards` = one Chip's Forge cost; `bRecordsLeaderboard` false); encounter: ally low health = `GeoHealthThresholdTrigger` (`WatchedClass` = `BP_Dummy`, `WatchedTag` = the ally's `AllySetup.Tag`, `Below`, `Ratio` 0.3 placeholder, `bRepeat`) → Cat line, death tips by player death `Count`, kill line on the boss's death
- [x] Tests: `GeoTrinity.Arena.SpawnAddReset` (wipe reset; the boss-death path needs a real boss defeat with loot, left to PIE). `NoLeaderboard` not written: proving it means writing the real leaderboard save under `Saved/`; the gate is one `if` in `MulticastRecordAttempt`
- [x] Run: build, then `Tools\Run_Tests.bat GeoTrinity.Arena`
- [x] Also written: `AGeoGrowingZone` (`Actor/Deployable/Zones/`); `UGeoLowHealthEventComponent` too, removed by slice 2b

## Slice 7 — Steps 11-12 (gems, Forge)
Docs first: `Gem/CLAUDE.md` (profile events), `Encounter/Trigger/CLAUDE.md` (two rows), `Encounter/Action/CLAUDE.md` (`GeoOpenGemsPageAction`).
- [x] `Gem/GeoGemProfileSave.h/.cpp` — native multicast `OnGemEquipped(EPlayerClass, int32 SocketIndex, FName GemId)` broadcast by a successful `Equip`; `OnGemCrafted(FName GemId, int32 Quantity)` by a successful `Craft`
- [x] `Encounter/Trigger/GeoGemEquippedTrigger`, `GeoGemCraftedTrigger` — bind the first local player's `UGeoGemSubsystem::GetProfile()` events in `OnArm`; event triggers (never fire for what was slotted before)
- [x] `Encounter/Action/GeoOpenGemsPageAction` — opens the Gems menu (loadout or Forge page) through a `GeoHUDInterface` call
- [ ] Editor: step 11 = the menu guide to the loadout (`AI/Pending/MenuGuide.md`, later; until then `GeoOpenGemsPageAction`), done on `GeoGemEquippedTrigger`; step 12 = the guide to the Forge, done on `GeoGemCraftedTrigger`, then `HostListen(TrainingRoomMap, "TrainingRoom")` (a `GeoLeaveTutorialAction`, or the pause-menu path reused)
- [x] Tests `GeoTrinity.Gems.ProfileEvents`. `GeoTrinity.Encounter.GemEquipped` not written: the trigger reads the first local player's `UGeoGemSubsystem`, which loads the real profile from disk; event-only by construction (binds in `OnArm`)
- [x] Run: build, then `Tools\Run_Tests.bat GeoTrinity.Gems` and `GeoTrinity.Encounter`
- [x] `Encounter/Action/GeoLeaveTutorialAction` → `UGeoGameInstance::LeaveTutorial()` (the pause menu's path)

## Slice 8 — Intro pages and the Cat (art last)
- [ ] Intro screens: layered images + drift data (`IntroBoard.md`); layer art for pages 1-2
- [ ] Landing level dressing; Cat hologram: mesh done (`SM_Cat`, `Mesh/generate_cat_mesh.py`), left: materials per slot (palette, open design above), placement; rig + animation → `CatAnimation.md`
- [ ] Cat animations: whistle (note glyph + tail sway), stunned (jaw drop, wide eyes, spin)
