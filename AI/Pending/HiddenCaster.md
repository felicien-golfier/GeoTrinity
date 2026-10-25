# HiddenCaster — pending

Goal: an encounter can cast hazard patterns and waves in a room with no boss (tutorial zone room and lateral waves), through an invisible caster it fires on demand.
Docs written: `Source/GeoTrinity/Public/Characters/CLAUDE.md` (tree, table, "HiddenCasterCharacter"), `Encounter/Action/CLAUDE.md` (row, "GeoActivateAbilityAction"), `Actor/Arena/CLAUDE.md` (`GeoDummyArena` row).
Grouping checked: one caster class next to `EnemyCharacter.h`, one action next to its siblings. The Training Room's existing caster (`BP_TutorialZoneCaster`, parent `AEnemyCharacter`, hidden + `bCanBeDamaged` false by Blueprint defaults) moves onto the new class (Editor below).

## Code (to implement)
- [x] `Characters/HiddenCasterCharacter.h/.cpp` — `AHiddenCasterCharacter : AEnemyCharacter`; constructor: `DoNotCreateDefaultSubobject(TEXT("CharacterWidgetComponent"))`, no skeletal mesh, hidden in game, indicator off; server BeginPlay `SetInvulnerable(true)`
- [x] `AI/GeoEnemyAIController.cpp` — no StateTree asset already ran nothing, no ensure; the gap was aggro (proximity aggro → `StartFight` → `SetInvulnerable(false)`): `AEnemyCharacter::CanBeAggroed()`, false on the caster, gates `InitializeAggro`
- [x] `Encounter/Action/GeoActivateAbilityAction.h/.cpp` — `bLatent = true`; `Arena`, `AbilityTag`, `bWaitForEnd = true`; activate through the boss's ASC by tag, bind the ability-ended delegate for that spec handle; refusal → `LogGeoEncounter` warning + `Finish()`; `OnCancel` cancels the ability and unbinds
- [x] Check a pattern ability with no montage and no mesh runs clean on the caster: `HandleAnimationMontage` and `UPattern` both guard a null `AnimInstance`
- [x] clang-format touched files; `GEOTRINITY_API` on both classes

## Editor / Blueprint
- [ ] `BP_HiddenCaster` (parent `AHiddenCasterCharacter`, not `BP_BaseEnemyBlueprintToInheritFrom`: no base StateTree); a child per room lists its abilities
- [x] Reparent `BP_TrainingZoneCaster` (renamed from `BP_TutorialZoneCaster` by `AI/Pending/Tutorial.md` slice 0) to `AHiddenCasterCharacter`; drop its `bHidden` / `bCanBeDamaged` overrides; keep its StateTree (`ST_Training_Zones`); update `training_room.py` to match
- [ ] Per casting room: an `AGeoDummyArena` with `BossClass` = that child, `ArenaTag` = the room tag, its `TargetPoint.*` points (with the tutorial map, `AI/Pending/Tutorial.md`)

## Tests (headless, `AI/Testing.md`)
- [x] Written and passing (2026-10-18 run, 62/62): `Private/Tests/GeoEncounterTests.cpp` + `GeoTestEncounter.h` (`AGeoTestCasterArena`) + `UGeoTestCastAbility` in `GeoTestPatternAbility.h`
  - `GeoTrinity.Characters.HiddenCaster` — spawned caster: `CanBeDamaged` false, collision off, hidden, no widget component
  - `GeoTrinity.Encounter.ActivateAbility` — ability activates on the arena's boss; with `bWaitForEnd` the next action runs only after the ability ends; without it, at once
  - `GeoTrinity.Encounter.ActivateAbilityRefused` — unknown tag / no boss: warning logged, action finishes, step goes on
  - `GeoTrinity.Encounter.ActivateAbilityCancel` — step left while waiting: ability cancelled, a late end ignored
- [x] Run: `Tools\Build_Editor.bat` then `Tools\Run_Tests.bat GeoTrinity.Encounter` and `GeoTrinity.Characters` (`GEO_NO_PAUSE=1`, editor closed)

## Verify
- [x] Parsed report: 62/62, the four HiddenCaster / ActivateAbility tests included

## Status 2026-10-25
C++ and 4 tests verified. Remaining: `BP_HiddenCaster` Blueprint and per-room `AGeoDummyArena` setup (editor).
