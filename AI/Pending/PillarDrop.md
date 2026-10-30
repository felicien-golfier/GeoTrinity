# PillarDrop — pending

Goal: the boss pillar drops in from above during the pattern's telegraph, solid only once it lands.
Priority: 2
Docs written: `Actor/Deployable/CLAUDE.md` ("Init phase"), `Actor/Deployable/Pillar/CLAUDE.md`, `AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md` ("SpawnPillar").

## Code (to implement)
- [x] `GeoDeployableBase.h/.cpp` — `Params.InitDuration`, `InitMontage`, `InitStartOffset`, `InitCurve`, `InitStartServerTime`, `IsInitialising()`, init start / tick / finish; push + collision timer moved to BeginPlay / `FinishInit`.
- [x] `GeoPillar.cpp` — constructor sets `InitStartOffset`.
- [x] `SpawnPillarPattern.h/.cpp` — spawn in `InitPattern` with `InitDuration`, `bAutoFinishInit` false; `EndPattern` releases them (`FinishInit`) after the judge, force-stop expires them.
- [x] Test `GeoTrinity.Gameplay.Deployable.InitPhase` in `GeoDeployableInitTests.cpp`; row in `Tests/Gameplay/CLAUDE.md`.
- [ ] `GeoDeployableBase.cpp` `FinishInit` — body only `if (bInitialising)`. Bug: `StartDelay <= TravelTime` → `InitDuration` 0 → no init; `EndPattern` still calls `FinishInit` → `SetCanBeDamaged(bDamageableAfterInit = false)` → pillar never dies, push + spawn cue + `Spawn` moment replayed.
- [ ] `GeoDeployableBase.cpp` `FinishInit` — restore `InitRestTransforms` directly instead of `ApplyInitPose(1.f)` (a curve ≠ 0 at 1 leaves the mesh offset).
- [ ] `GeoDeployableBase.cpp` `StartInit` — `CueParams.Normal.Y = GeoLib::GetServerTime(GetWorld()) - InitStartServerTime;` (today 0: on a client the `InitCue` telegraph starts on arrival, outlives the landing by the net delay; a late joiner sees a full one).

## Editor / Blueprint (by hand)
- [x] `BP_Pillar_Deployable` — `InitCurve` = `CF_PillarDrop` (1 → 0, ease-in), `InitCue` = `GameplayCue.Generic.ZoneIndicator` (Damage).
- [ ] Decide with the user: pillar `InitCue` (`Damage`, pillar `Size`) draws inside the pattern's `InitCue` (`Damage`, `SpawningZoneSize`) at the same spot → zone layer stacks them (`StackBrightness`, second outline): keep as the footprint, or clear the pillar's `InitCue`.
- [ ] Decide with the user: `InitMontage` + `PlayInitMontage` have no asset using them (YAGNI, `AI/CodingStyle.md`): keep or remove (then `Deployable/CLAUDE.md`, `Pillar/CLAUDE.md` lose it).
- [ ] `BP_Pillar_Deployable` — mesh = the look wanted (no bomb mesh exists); optional `InitMontage` on a skeletal mesh; `FXMap[Init]` sounds; tune `InitStartOffset`.

## Tests
- [ ] `GeoDeployableInitTests.cpp` — add: auto-finish (`bAutoFinishInit` true ends by itself at `InitDuration`); `FinishInit` on a non-initialising deployable leaves it damageable, no second spawn cue (fails on the old code); landing restores the rest transform with a curve ≠ 0 at 1; `InitCue` `Normal.Y` = time since `InitStartServerTime`.
- [ ] `USpawnPillarPattern` — natural end releases its pillars (live, damageable), force-stop expires them; with `StartDelay <= TravelTime` the pillars stay damageable. Lightest layer per `AI/Testing.md`; row in `Tests/Gameplay/CLAUDE.md`.
- [ ] Check: boss destroyed mid-telegraph (no `EndAbility` → no `EndPattern`?) leaves inert pillars; if so, expire them from the pattern's teardown.

## Verify
- [x] `Tools\Run_Tests.bat GeoTrinity.Gameplay.Deployable` + `GeoTrinity.Tutorial.` (31 green)
- [ ] After the items above: Editor build, `Tools\Run_Tests.bat GeoTrinity.Gameplay.Deployable`, then `GeoTrinity.Tutorial.` (tutorial gate, root `CLAUDE.md`)
