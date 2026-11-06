# ZoneTelegraph — pending

Goal: every effect zone shows its telegraph before it lands; the zone knows nothing of it, the ability's wind-up is the warning. The Zones room's zones stop being spawned by an action and become zone-pattern abilities.
Priority: 1.
Docs written: `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/Zone/CLAUDE.md`, `Source/GeoTrinity/Public/Encounter/Action/CLAUDE.md`, `Source/GeoTrinity/Public/Actor/Deployable/CLAUDE.md`, `AI/Tutorial/CLAUDE.md`, `AI/Tutorial/Steps.md` (step 7).

## Read map
- `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/Zone/ZonePattern.h`, `Source/GeoTrinity/Private/AbilitySystem/Abilities/Pattern/Hazard/Zone/ZonePattern.cpp` — add `bZoneUntilStopped`
- `Source/GeoTrinity/Public/Encounter/Action/GeoActivateAbilityAction.h`, `Source/GeoTrinity/Private/Encounter/Action/GeoActivateAbilityAction.cpp` — add `bHeldUntilStepLeft`
- (GeoSpawnZoneAction, to delete) (already gone from `Source/`: only leftover includes / mentions remain)
- `Source/GeoTrinity/Private/Actor/Deployable/Zones/GeoEffectZone.cpp` — unchanged (growth starts at its BeginPlay = the landing); header comment mentions the spawner
- `AI/Python/Tutorial/Level/tutorial_encounters.py` (`zone()`, `zones_steps`, `ZONE_*`) — rebuild to abilities
- `Source/GeoTrinityEditor/Private/Tests/Gameplay/Encounter/GeoEncounterSpawnTests.cpp` (+ files including GeoSpawnZoneAction.h: Summon, EnemySetup, Trigger, Jump, Flow, Cat, Ability, GrowingZone, DeployableStartHealth tests) — drop the include / port `SpawnZone*` tests
- `Source/GeoTrinityEditor/Private/Tests/Tutorial/GeoTutorialRoomStages.cpp`, `Source/GeoTrinityEditor/Private/Tests/Tutorial/GeoTutorialBot.cpp`, `Source/GeoTrinityEditor/Private/Tests/Tutorial/CLAUDE.md` (coverage table) — `ZonesCamp` windows, `DefaultZoneClass` use
- `Config/Tags/GeoGameplayTags.ini` (l.110) — `TargetPoint.ZoneSpawn` comment names the action
- `EnteredEvent` dropped (absent from the code, user confirmed): remove any mention left in docs / rules

## Code (to implement)
- [x] `UZonePattern::bZoneUntilStopped` (lingering only, `LifeDrainMaxDuration` 0): `StartPattern` spawns the zones and does not call `EndPattern()`; keep the spawned zones in a weak list (reset in `InitPattern`); `EndPattern(true)` `Recall()`s each valid one. `bHasHazard` stays false.
- [x] `UGeoActivateAbilityAction::bHeldUntilStepLeft`: non-latent (`bLatent` false path), `HoldUntilStepLeft`, `OnCancel` cancels the ability (also on abort); run again while held cancels the previous firing first. `IsDataValid`: not combined with a waiting expectation.
- [x] Delete `GeoSpawnZoneAction`; `AGeoEffectZone` header comment: spawned by a zone pattern.
- [x] Update every test using the action; keep the held/cancel coverage on `GeoActivateAbilityAction`.

## Tests (per `AI/CodingStyle.md`)
- [ ] `bZoneUntilStopped`: zone absent during the wind-up, present at `StartDelay`, pattern still alive; `EndPattern(true)` recalls it.
- [ ] Zone judge: no effect applied during the wind-up, applied after landing (growth clock starts at landing).
- [ ] `bHeldUntilStepLeft`: Finish at once, ability cancelled on step left / rewind / abort, previous firing cancelled on re-run.

## Editor / Blueprint (by hand or script)
- [x] Three abilities `GA_Tutorial_ZoneHeal|Shield|Damage` (`tutorial_zones.py`; plain `UPatternAbility`, `Target` = `TargetPoint` tag `TargetPoint.Tutorial.Zone<Colour>`) each with its own `BP_TutorialZone<Colour>` (`UZonePattern`; one live instance per class): `bZoneUntilStopped`, `ZoneParams` (`Size`, `Color`, `SecondaryColors`, `Attitude` all, grow fields on Damage from the rows), effects from the old action's values, `StartDelay` = the ability's default fire delay, no row. Register like the other encounter-fired abilities (see how `GA_LateralWave` is).
- [x] Remove `ZoneColors` steps' `GeoSpawnZoneAction`s from the map (script below).

## Scripts
- [x] `tutorial_encounters.py`: `zone(color)` → held `GeoActivateAbilityAction` on the colour's ability, `ArenaTag` Zones; run `sync`/replay per `AI/Python/Tutorial/CLAUDE.md`, map gate first (`AI/Python/Level/CLAUDE.md`).

## Verify
- [ ] Quick suite; Tutorial runs at risk (the Zones room changes): `Path.ZonesCamp` windows shift by the fire delay (same for every zone, so a plain shift of the bot's windows, rows untouched). Update the coverage table.
