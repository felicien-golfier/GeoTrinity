# SpawnEnemyAbility — pending

Goal: one ability spawns enemies (`UGeoSpawnEnemyCharacterAbility`, with the Arena behind it); a boss's summon is two separate abilities, a zone pattern then a spawn; ally and dummy are the same class with different assets.
Priority: 1
Replaces `SummonPattern.md` (deleted: built by commit 8c42ad08; its open checks are below).
Docs written: `Abilities/Boss/CLAUDE.md` ("Summon", "GeoSpawnEnemyCharacterAbility"), `Abilities/Pattern/Hazard/Zone/CLAUDE.md`, `Abilities/Base/CLAUDE.md` (`RequiresArena`), `AbilitySystem/Lib/CLAUDE.md` (`PatternTargets`), `Characters/CLAUDE.md` (`Death()`, "Enemy setup", "Summoned dummy"), `AI/Tutorial/Steps.md`, `AI/Python/Tutorial/CLAUDE.md`, `Tests/Gameplay/Characters/CLAUDE.md`.

## Read map
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Common/GeoPlaceDeployableAbility.cpp` — the `Fire` shape to copy. `GeoSpawnOnDeathAbility.cpp` and `SpawnAddPattern.cpp` (what goes) are already gone from the code.
- `Source/GeoTrinity/Public/AbilitySystem/Abilities/Base/GeoGameplayAbility.h:358-366` — `bUseGeneralChargeTimeForFireDelay`, `FireDelay` private; `GeoGameplayAbility.cpp:278` `ScheduleFireTrigger`.
- `Source/GeoTrinity/Public/AbilitySystem/Lib/GeoTarget.h`, `Source/GeoTrinity/Private/AbilitySystem/Lib/GeoAbilitySystemLibrary.cpp:795` `PickTargets`; `Pattern.h:82` `GetTargetLocations`; `GeoAbilitySystemComponent.cpp:532` `FindPatternByClass`.
- `Source/GeoTrinity/Private/Characters/GeoCharacter.cpp:263` `Death()`; `EnemyCharacter.cpp:136` `DeathLogic`.
- `Private/AbilitySystem/Lib/GeoGameplayTags.cpp:117` — native tag shape.
- `Abilities/Base/GeoChannelBeamAbility.cpp:66` — an `OnGiveAbility` override. Engine: `AbilitySystemComponent_Abilities.cpp:550-558` creates the instance before `OnGiveAbility` (`ReplicateNo`); `AbilitySystemComponent.h:741` `GenericGameplayEventCallbacks` (exact tag).
- `AI/Python/Tutorial/Level/tutorial_miniboss.py` — `build_summon_pattern`, `build_summon`, `build_summon_ally`, `summon_setup`, `build_tree`; `tutorial_encounters.py:75` `MINIBOSS_ABILITIES`; `AI/Python/Level/CLAUDE.md` map gate.
- `Config/DefaultEngine.ini:197` — `[CoreRedirects]`.
- Tests: `Tests/Gameplay/Characters/GeoSummonTests.cpp`, `Source/GeoTrinityEditor/Private/Tests/Gameplay/Fake/GeoTestPatternAbility.h` (l.7,10,327,374), `Source/GeoTrinityEditor/Private/Tests/Tutorial/GeoTutorialRoomStages.cpp` (l.1460-1480).

## State (session stopped on the usage limit)
Done, unbuilt and untested: all C++ below, `Event.Character.Dying` + `Ability.Spell.TutorialSpawnDummy` in the tag ini, the redirects, the test rewrite (`GeoSummonTests.cpp`: `SummonedDummy`, `Abilities.SpawnEnemyCharacter`, `Abilities.PatternTargets`; fakes in `GeoTestPatternAbility.h`), clang-format on touched files. Not done: scripts (`tutorial_miniboss.py` / `tutorial_encounters.py` untouched), build, quick suite, tutorial runs, audit. Next: build via `test-fixer` first (possible compile fixes: `ArbitraryCharacterZ` include in the new .cpp, `GenericGameplayEventCallbacks` access, test fakes). `has_state` for the `Land` patch needs `EditorData.SubTrees` readable from Python; verify when running. `GeoTutorialRoomStages.cpp` has a bot edit ("walks up to the boss") that is not from this task. "No-target launch keeps previous locations" is not tested (it only follows from `PatternAbility` skipping the multicast).

## Code (to implement)
- [x] `Abilities/Boss/GeoSpawnEnemyCharacterAbility.h/.cpp` — per doc: `EnemyClass`, `Setup`, `bSpawnOnOwnerDying`, `Fire`, `RequiresArena()`, `OnGiveAbility` bind. Spawn location `FVector(Location, ArbitraryCharacterZ)`.
- [x] `GeoGameplayAbility.h` — `bUseGeneralChargeTimeForFireDelay` and `FireDelay` protected, for the constructor above.
- [x] `GeoTarget.h` + `PickTargets` — `EGeoTarget::PatternTargets`, `TSubclassOf<UPattern> PatternClass` (`EditCondition`); one actor-less pick per location.
- [x] `GeoGameplayTags` + `Config/Tags/GeoGameplayTags.ini` — native `Event.Character.Dying`.
- [x] `GeoCharacter.cpp` `Death()` — send the event after the guard, before `bIsDead = true`.
- [x] Delete `GeoSpawnOnDeathAbility.h/.cpp`, `SpawnAddPattern.h/.cpp`.
- [x] `DefaultEngine.ini` `[CoreRedirects]` — `ClassRedirects` `GeoSpawnOnDeathAbility` → `GeoSpawnEnemyCharacterAbility`, `SpawnAddPattern` → `ZonePattern`; `PropertyRedirects` `AllyClass` → `EnemyClass`, `AllySetup` → `Setup`. So `GA_TutorialSpawnAlly` and `BP_TutorialPattern_Summon` still load.
- [ ] Grouping: nothing to fold (`Boss/` has its `CLAUDE.md`); `UGeoSpawnEnemiesAction` stays (no arena; the shared caster refuses `RequiresArena()`).

## Scripts (`AI/Python/Tutorial/Level/`)
- [x] `tutorial_miniboss.py` (written, not yet run):
  - `build_summon_pattern`: parent `/Script/GeoTrinity.ZonePattern`; no `AddClass`, `AddSetup`.
  - `build_summon_ally`: parent the new class; `EnemyClass`, `Setup`, `bSpawnOnOwnerDying` true, `Target` `(Mode=Self)`; tags = `SUMMON_ALLY_TAG` only, **no `PASSIVE_TAG`** (it would spawn the ally at once).
  - New `build_spawn_dummy`: `GA_TutorialSpawnDummy`, new tag `Ability.Spell.TutorialSpawnDummy` (sibling, in the ini), `EnemyClass` `BP_Dummy`, `Setup` = `summon_setup()`, `Target` `PatternTargets` = `BP_TutorialPattern_Summon`, `waves.register`. Runs after `build_summon`.
  - `build_tree`: new `Land` state (`FireAbility`, new tag), `Summon` → `Land` → `CountSummons`, in the fresh path and, when `Land` is missing, on the existing tree (`clear_transitions` on `Summon`), so hand tuning (`CyclesRequired`) stays.
- [x] `tutorial_encounters.py:75` (written, not yet run) — add the new tag to `MINIBOSS_ABILITIES`, or `Land` fails silently; run through the map gate; check the report's `kept by hand` holds no `BossSetup`.
- [ ] Run order: `tutorial_miniboss.py`, `tutorial_encounters.py`. Then grep `Content` and scripts for leftovers of the deleted classes.
- [x] `AI/Python/Tutorial/Runtime/pie_tutorial_miniboss.py` (checked: finds by actor `Tag`, fine) — still finds summons by `Tag`; fix if not.
- [ ] From `SummonPattern.md`: confirm a summon on `BP_Dummy`'s class tree never moves off its zone, else give it no tree; `DummyZone*` rows are read at the difficulty column (flat today = no change).

## Tests (`GeoSummonTests.cpp`, `AI/CodingStyle.md` "Tests")
- [ ] Rewrite fakes: `UGeoTestSpawnOnDeathAbility`, `UGeoTestSpawnAddPattern` → children of the new class; a plain zone pattern where a launch is needed.
- [ ] `GeoTrinity.Abilities.SpawnEnemyCharacter` (replaces `SummonAddSetup`): fired → one enemy per `Target` location, in the caster's arena, with `Setup`; ability ended in the same call.
- [ ] `PatternTargets`: the launch's locations after the pattern ended and the player moved; none before any launch; no-target launch keeps the previous; another pattern on the caster does not interfere.
- [ ] `GeoTrinity.Characters.SummonedDummy`: dummy dies → one ally at its place, same arena, setup's team; not before; two dummies → one each; destroyed without dying → none; an `OnCharacterDied` listener already sees the ally; `bSpawnOnOwnerDying` false → nothing on death; the ability isn't active while waiting.

## Verify
- [ ] `Tools\Build_Editor.bat`, quick suite via `test-fixer`.
- [ ] Tutorial runs at risk (miniboss changed): `GeoTrinity.Tutorial.FullRun` (also proves `Land` with an ability that ends inside `EnterState` doesn't stick), `Path.MinibossWipe`, `Path.MinibossWaveLanes`, `Path.GemsForge`, `Path.GuideStray`, `Path.FinalExit`.
