# EncounterSpawnSetup — pending

Goal: fewer tutorial assets: enemy variants and plain zones are written inline on the encounter (one `BP_Dummy` + a setup, a zone action) instead of one Blueprint / ability per variant.
Docs written: `Characters/CLAUDE.md` ("Enemy setup", `GeoSummonedDummy`, `HiddenCasterCharacter`), `Encounter/Action/CLAUDE.md` (`GeoSpawnEnemiesAction`, `GeoSpawnZoneAction`), `Encounter/Trigger/CLAUDE.md` (`WatchedTag`), `Actor/Arena/CLAUDE.md` (`SpawnAdd`), `Actor/Deployable/CLAUDE.md` (lifecycle), `AI/Tutorial/CLAUDE.md` (Runtime), `AI/Tutorial/Steps.md` (steps 4, 5, 7), `AI/Python/Level/CLAUDE.md`.
Training Room: no backward compatibility owed; it may break until its own redesign (`Tutorial.md` slice 0, "Training Room map design").
Grouping checked: setup struct next to `EnemyCharacter.h`; zone action beside its spawn sibling in `Encounter/Action/`; the two dummy StateTrees into one `/Game/AI/Dummy/` folder.

## Code (to implement)
- [x] `Characters/GeoEnemySetup.h` — `FGeoEnemySetup` (fields per `Characters/CLAUDE.md` "Enemy setup")
- [x] `Characters/EnemyCharacter.h/.cpp` — `ApplySetup(FGeoEnemySetup const&)` (server, pre-`FinishSpawning`: `TeamId`, `ResetToFullLifeWhenReachingZero`, `StateTree` if set, append `StartupAbilityTags`, `Tags.Add(Tag)`, keep `MaxHealth` override + `StartHealthRatio`); `InitGAS` applies them after `Super::InitGAS`; one private `ResetAttributes()` (`InitializeDefaultAttributes` + `MaxHealth` override) used by the reset-at-zero and `ResetForNewAttempt` paths
- [x] Check: possession runs in `APawn::PostInitializeComponents`, inside `FinishSpawning`, so `OnPossess` reads the setup's StateTree (read in the engine source)
- [x] `Encounter/Action/GeoSpawnEnemiesAction.h/.cpp` — `Setup` property; spawn = `SpawnActorDeferred` → `ApplySetup` → `FinishSpawning`
- [x] `Encounter/Action/GeoSpawnZoneAction.h/.cpp` — new, per `Encounter/Action/CLAUDE.md`; `EffectDataArray`, `Params`, `Team`, `PurposeTag`, `ArenaTag`; latent until cancelled, `OnCancel` expires the zone
- [x] Check: `FullySpawnDeployable` refuses a null owner / non-pawn instigator, so the action spawns deferred and sets `Owner` = `Instigator` = the zone itself, then `InitInteractable`
- [x] `Encounter/Trigger/GeoActorDiedTrigger.h/.cpp`, `GeoHealthThresholdTrigger.h/.cpp` — `WatchedTag` (`FName`), tested beside `WatchedClass`
- [x] `Actor/Arena/GeoArena.h/.cpp` — `SpawnAdd(Class, Transform, FGeoEnemySetup const* Setup = nullptr)`, deferred spawn when a setup is given
- [x] `Characters/GeoSummonedDummy.h/.cpp` — `AllySetup`, passed to `SpawnAdd`
- [x] `Config/Tags/GeoGameplayTags.ini` — `TargetPoint.ZoneSpawn` (where an encounter's zone appears), `TargetPoint.Tutorial.Ally` (where a tutorial room spawns its ally)

## Scripts
- [x] `AI/Python/Level/tutorial_assets.py` — drop `build_dummies` / `DUMMIES`; build `/Game/AI/Dummy/ST_Dummy_Shooter` (from today's `ST_Tutorial_Shooter` shape) and `ST_Dummy_Chaser` (from `ST_Training_Chase`)
- [x] `AI/Python/Level/tutorial_encounters.py` — every `b.spawn` uses `BP_Dummy` + a setup helper (`Team`, `bResetToFullLife` false, `StateTree`, `AbilityTags`, `MaxHealth` from `CT_Tutorial`, `StartHealthRatio`, `Tag`); steps 4-5: drop the placed `Tut_Deploy_Ally`, `Tut_Special_Ally`, `Tut_Special_Dummy`, add a `TargetPoint.Tutorial.Ally` point per room, spawn them on step enter (`Count` 1, spawn last in `OnEnter`), triggers watch `BP_Dummy` + `WatchedTag`; step 3 kill triggers watch their `Tag` too; step 6 chaser / shooter / mover as setups (mover: no StateTree until its patrol); a `b.zone(...)` helper for `GeoSpawnZoneAction`
- [x] `AI/Python/Level/training_room.py` — point `BP_TrainingChaser` / `BP_TrainingShooter` at the two `ST_Dummy_*`; drop `ST_Training_Bullets`, `ST_Training_Chase` creation (rest left as is)
- [ ] Needs the editor closed: run both tutorial scripts (headless), then `training_room.py build_assets()`

## Editor / Blueprint (by script above, or by hand)
- [ ] Delete `/Game/Characters/Enemies/Tutorial/` (`BP_TutorialDummy`, `BP_TutorialChaser`, `BP_TutorialShooter`, `BP_TutorialMover`, `BP_TutorialHurtAlly`), `/Game/AbilitySystem/DefaultAttributes/GE_TutorialHurtAllyAttributes`, `/Game/AI/Training/ST_Tutorial_Shooter`, `ST_Training_Bullets`, `ST_Training_Chase` — after the scripts no longer reference them; fix up redirectors
- [ ] Never created (were planned): `BP_HiddenCaster_TutorialZones` (now `BP_HiddenCaster_TutorialWaves`, waves only), `BP_TutorialMinibossAlly`, `BP_TrainingMover`
- [ ] Re-save `/Game/Maps/Tutorial` and `DraftMap`

## Tests (`Private/Tests/GeoEncounterTests.cpp`, test world of `GeoTestWorld.h`)
- [ ] `GeoTrinity.Encounter.SpawnEnemiesSetup` — spawned enemy has the setup's team, tag, `StartupAbilityTags` granted, `MaxHealth` override and `StartHealthRatio`; with `bResetToFullLife` false it dies at zero
- [ ] `GeoTrinity.Characters.SetupHealthReset` — `bResetToFullLife` true: at zero it comes back at the overridden `MaxHealth`, not the class's
- [ ] `GeoTrinity.Encounter.WatchedTag` — died / health triggers ignore a same-class character without the tag
- [ ] `GeoTrinity.Encounter.SpawnZone` — zone on the tagged point with the action's size and team; cancel expires it
- [ ] `GeoTrinity.Arena.SpawnAddReset` still green with a setup passed
- [ ] Run: build (Editor target), then `Tools\Run_Tests.bat GeoTrinity.Encounter`, `GeoTrinity.Characters`, `GeoTrinity.Arena`, `GeoTrinity.Tutorial`

## Verify
- [ ] `AI/Python/Runtime/pie_tutorial_flow.py` re-run (only when the user asks for PIE), steps 3-6 still complete
