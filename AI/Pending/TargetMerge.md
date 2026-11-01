# TargetMerge — pending

Goal: one targeting system for every ability: everything starts with a location, a direction is derived from it; a `Target` param replaces `EProjectileTarget` and the pattern `Spot`.
Priority: 0
Docs written: `AbilitySystem/Lib/CLAUDE.md` ("Targeting"), `Abilities/Base/CLAUDE.md` ("Target", "Pattern target"), `Abilities/Boss/CLAUDE.md`, `Abilities/Pattern/CLAUDE.md`, `Pattern/Projectile/CLAUDE.md` ("Salve"), `Pattern/Hazard/CLAUDE.md` ("LateralWave"), `Pattern/Hazard/Zone/CLAUDE.md`, `Abilities/Damaging/CLAUDE.md`, `Components/CLAUDE.md`, `Actor/Arena/CLAUDE.md`, `Tests/Gameplay/CLAUDE.md`, `AI/Python/Tutorial/CLAUDE.md`.
Then: `ZonePatternSpawns.md` (pillars, adds) on top; its names already follow this design.

Behaviour: none on the happy paths. Changes only where a target is missing: a `TargetPoint` with no point / room launches nothing (was: from the caster).

## Code
- [ ] New `AbilitySystem/Lib/GeoTarget.h` — `EGeoTarget` (`Forward`, `AllPlayers`, `RandomPlayers`, `Boss`, `TargetPoint`), `FGeoTarget` (`Mode`; `Distance`; `Count`, `ExtraBelowHealth` (`TArray<float>`), `MinScatterRadius`, `MaxScatterRadius`; `TargetPointTag`; each `EditCondition` + `EditConditionHides` on `Mode`), `FGeoTargetContext`, `FGeoTargetPick` (plain struct). Check the names are free.
- [ ] `GeoAbilitySystemLibrary.h/.cpp` — delete `EProjectileTarget`; `PickTargets`, `ResolvePicks`, `GetTargetLocations`, `GetTargetDirections(Target, Context, Origin)` (zero length = `Context.Yaw`); `SpawnProjectileSpread` takes `FGeoTarget` + context. `RandomPlayers` logic from `GeoSpawnPillarAbility::FillPillarTargets` (health steps from `ExtraBelowHealth` instead of the hard-coded .5 / .2). `TargetPoint` from `UPatternAbility::GetFireOrigin2D`.
- [ ] `GeoGameplayAbility.h/.cpp` — `UPROPERTY FGeoTarget Target` (default `Forward`, `Distance` 0); `MakeTargetContext()`; `SpawnRemoteProjectiles` takes `FGeoTarget`.
- [ ] `GeoProjectileAbility.h/.cpp`, `GeoAutomaticProjectileAbility.h/.cpp` — own `Target` removed, read the base's.
- [ ] `GeoShieldBurstPassiveAbility.cpp:142` — `FGeoTarget` `Forward`.
- [ ] `PatternAbility.h/.cpp` — picks member; `BeginPreLaunch` picks + marks (base `AddPreLaunchCue` per pick actor, caster when none); `LaunchPattern` picks if none, resolves, empty → end the ability (not cancelled) and return; payload origin / yaw per `LandsOnTarget` (`Base/CLAUDE.md`, "Pattern target"); multicast with the locations; picks cleared. `GetFireOrigin2D` = first resolved location when landing, else `Super`. `TargetPointTag` → `TargetPointTag_DEPRECATED` (loads the saved value) + `PostLoad`: valid and `Target.Mode` `Forward` → `Mode` `TargetPoint`, `Target.TargetPointTag` = it. Removed after the resave below.
- [ ] `GeoAbilitySystemComponent.h/.cpp` — `PatternStartMulticast(Payload, Class, TargetLocations, Data)`; `Pattern->SetTargetLocations` before `InitPattern`.
- [ ] `Pattern.h/.cpp` — `SetTargetLocations`, `GetTargetLocations` (empty = `{Payload.Origin}`), `virtual bool LandsOnTarget() const` false; true in `ZonePattern`, `DevastatingWavePattern`, `LateralWavePattern`.
- [ ] `GeoPeriodicPatternAbility.h/.cpp` — `Target` member and `CreatePatternData` override removed; constructor `Target.Mode` = `AllPlayers`; an empty launch skips but keeps the schedule.
- [ ] `ProjectilePattern.h/.cpp` — `FProjectilePatternData` removed (only `Yaws`, only the salve); `USalvePattern` yaws from `GetTargetLocations()` − `Payload.Origin` (zero = `Payload.Yaw`).
- [ ] `GeoSpawnPillarAbility`, `GeoSummonAddAbility`: untouched here (`ZonePatternSpawns.md` deletes them).
- [ ] Comments naming `TargetPointTag` on the ability: `GeoActivateAbilityAction.h:46`, `PatternAbilityParams.h:23`.
- [ ] Editor build (`GEO_NO_PAUSE=1`, editor closed).

## Asset migration (headless, after the build)
- [ ] Load and save every `UPatternAbility` Blueprint carrying `TargetPointTag`: `GA_DevastatingWave`, `GA_LateralWave`, `Training/GA_Training_{BurstDamage,BurstHeal,DamageOverTime,DamageReduction,HealOverTime,Lethal}` (from a `grep -la TargetPointTag` over `Content/`); report each one's `Target` after. Then remove `TargetPointTag_DEPRECATED` + `PostLoad`, build again.
- [ ] `GA_TutorialAllySalve` (only asset storing a non-default `EProjectileTarget`): set by the `tutorial_miniboss.py` re-run below.

## Scripts
- [ ] `AI/Python/Tutorial/Level/tutorial_miniboss.py:160,187` — set `Target` struct (`mode` `ALL_PLAYERS` / `BOSS`) instead of `unreal.ProjectileTarget`. Re-run headless.
- [ ] `AI/Python/Tutorial/Level/tutorial_waves.py:101` — `Target` (`TargetPoint`, `TargetPointTag` = `WAVE_START_TAG`) instead of `TargetPointTag`. Re-run headless.

## Tests
- [ ] New `Tests/Gameplay/GeoTargetTests.cpp` (`GeoTrinity.Abilities.Target.*`): `Forward` = origin + yaw × distance, 0 = origin; `AllPlayers` one per live player; `Boss` none outside a fight / boss dead; `TargetPoint` its tagged point in the room, none + ensure without; `RandomPlayers` seeded and stable, count follows `ExtraBelowHealth` and not the player count, one per player then scatter within the radii, no player = empty; a gone actor resolves to its `Location`, a live one follows it; zero direction = yaw.
- [ ] Same file, pattern side: `LandsOnTarget` true → payload origin = first location; false → caster origin, yaw towards the first; empty → no launch, ability ended; `PreLaunchCue` once per pick actor.
- [ ] `GeoPatternTests.cpp:135` — salve aim per `EGeoTarget` through `GetTargetLocations` (fake `GeoTestPatternAbility.h:163` `GetAimYaws` rewritten, no pattern data).
- [ ] `GeoEncounterTests.cpp:1033` + `Fake/GeoTestPatternAbility.h:152` `SetPointTag` — sets `Target` (`TargetPoint`).
- [ ] Tutorial bot: no room or step change, coverage table unchanged.

## Verify
- [ ] `Tools\Run_Tests.bat GeoTrinity.Abilities.Target`, `GeoTrinity.Pattern`, the encounter tests.
- [ ] `Tools\Run_Tests.bat GeoTrinity.Tutorial.` (tutorial gate, whole set), `FullRun` green.

## Not in this change
- `UGeoSweepBeamAbility` → a `Target` (arena centre point) + seeded angle offset: ask the user once this lands.
