# CueToZoneLayer — pending - TO BE SIMPLIFIED (Can't we "just" add a Cue that calls a zone with params.)

Goal: a footprint (circle, ray, marker on the ground) is never drawn by a cue's own Niagara or material: the zone layer draws it. A gameplay cue stays the way to launch one, so a Blueprint (a deployable's moment, a pattern cue, a test) picks a zone per case and no C++ owner forces one.
Priority: 2
Docs written: `Actor/Deployable/CLAUDE.md` ("Zone through a cue"), `Actor/Deployable/Pillar/CLAUDE.md`, `Actor/Deployable/BombZone/CLAUDE.md`, `Abilities/Common/CLAUDE.md`. Per undecided candidate, its owner's `CLAUDE.md` is written when the user decides it.
Supersedes `DeployableTelegraph.md` (its forced `LifeTelegraph` / `BlinkTelegraph` on the deployable is dropped: deployables stay versatile).

Behaviour: none. Same circles, colours, times.

## Decided (2026-11-01)
- A deployable has no zone of its own. Any of its moment cues (`InitCue`, `SpawnCue`, `BlinkingCue`, `ExplodeCue`, `ExpireCue`, `RecallCue`) set to a child of `UGeoZoneIndicatorCue` launches one; unset = none. Pillar and hex bomb keep `GC_ZoneIndicator` as it is: nothing to migrate for them.
- `UGeoZoneIndicatorCue` stays (name and tag `GameplayCue.Generic.ZoneIndicator` too: no reparent, no redirect). It gains Blueprint-editable defaults; its cue params contract is unchanged (`RawMagnitude` radius, `Normal.X` lifetime, `Normal.Y` elapsed).
- A Blueprint that is not a zone cue (any GC, any actor Blueprint) draws through a Blueprint-callable function: same path as the cue.
- Rule: a cue's own Niagara / material never draws a footprint; a cue may launch the zone layer.

## Read map
- `Source/GeoTrinity/Public/World/ZoneLayer/CLAUDE.md` — what the layer draws (`Circle`, `Beam`, `Ring`, fills, layers)
- `Source/GeoTrinity/Public/Characters/Component/FX/GeoIndicatorComponent.h/.cpp` — `FGeoIndicatorState`, `SpawnIndicator` (the one local draw path)
- `Source/GeoTrinity/Public/World/ZoneLayer/GeoZoneVisualComponent.h` + `GeoZoneShape.h` — `UCLASS(ClassGroup = (Geo))` only today: no Blueprint exposure
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Common/GeoZoneIndicatorCue.cpp` — the cue today (round only, Telegraph layer, colour mask from cue params)
- `Source/GeoTrinity/Private/Actor/Deployable/GeoDeployableBase.cpp` ~742-777 — `GetGenericCueParams` (location, `RawMagnitude` = `Params.Size`), spawn / blink params (`Normal.X` = `LifeDrainMaxDuration` / `BlinkDuration`)
- `Source/GeoTrinity/Private/AbilitySystem/Lib/GeoAbilitySystemLibrary.cpp` ~480-500 — `ExecuteGeoCue`, `SetCueMeaningColors`, `GetCueColors`
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Pattern/Hazard/Zone/ZonePattern.cpp` `InitPattern`, `RemoveIndicators`, `ExecuteGameplayCue` override — a sibling already launching indicators
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Common/GeoDeployTargetCue.cpp`, `GeoDeployAbility.cpp` ~221, ~280 — landing marker
- `Source/GeoTrinity/Private/AbilitySystem/Abilities/Base/PatternAbility.cpp` ~93-190 — `PreLaunchCue` on marked actors
- `AI/VFX.md` "Meaning Colours" — says the devastating wave "keeps its own system": stale (zone-layer only, `Hazard/CLAUDE.md`); fix in this change

## Inventory (code read; `Content/` cue Blueprints not opened: "asset?" = read the Blueprint before touching)
| # | Cue / system | Fired by | Draws | Verdict |
|---|---|---|---|---|
| 1 | `GC_ZoneIndicator`, `BlinkingCue` of `BP_Pillar_Deployable` | deployable | blast circle, `Params.Size`, `BlinkDuration` | keep as is (already the model) |
| 2 | same cue, `SpawnCue` + `BlinkingCue` of `BP_HexBomb_Deployable` (`GetSpawnCueParams` override in `AGeoBombZone`) | deployable | circle, `Neutral` then `Damage` | keep as is |
| 3 | `InitCue` of `BP_TileCarveRayPattern` | pattern | ray telegraph | asset? `UZoneHazardPattern` already telegraphs: clear the cue if it only repeats it |
| 4 | `AGeoDeployTargetCue` (`GameplayCue.Ability.Deployable.Target`, `GameDataSettings::DeployTargetCue`) | `UGeoDeployAbility`, local | landing marker, follows `GetPendingDeployLocation()`, then the projectile | asset? if its look is a circle: draws through the layer (a `UGeoZoneVisualComponent` default subobject on the cue actor, `Circle`, `Telegraph`); the cue stays |
| 5 | `GC_Marked` (`Status.Marked`, `M_MarkedCircle`) via `PreLaunchCue` | `UPatternAbility::AddPreLaunchCue` on each marked actor | circle on the marked actor | asset? re-author as a child of the zone cue (stripes come from the layer); `M_MarkedCircle` deleted |
| 6 | `GC_Targeted` (`Generic.Targeted`) | asset? | asset? | decide after reading it |
| 7 | `Boss.Pillar.ShowDirection` | pillar drop (`PillarDrop.md`) | asset? | decide: a `Ray` zone cue if it is a line on the ground |
| 8 | `Ability.Player.Square.SacrificeBeam`, `FireCue` of `GeoSacrificeDetonateAbility` / `GeoDetonateWallsAbility` | ability, local | "detonation ray" at `Endpoint` | decide: a short `Ray` zone cue if the ray is a footprint; keep its Niagara sparks |
| 9 | `FireCue` of `GeoChargeBeamAbility`, `GeoAutomaticFireAbility` | ability | muzzle / shot flash | keep (decoration) |
| 10 | `TurretRecall` + `NS_Square_RurretRecall_Beam` | turret `RecallCue` | ribbon between two bodies | keep (motion, not a footprint) |
| 11 | `Explosion`, `Damage`, `Heal`, `Sound`, `OneShotCue`, deployable `ExplodeCue` / `ExpireCue` bursts | various | bursts, sparks, numbers | keep |
| 12 | `StartCue` / `InitCue` of `UZonePattern`, `UBeamPattern`, `UDevastatingWavePattern` | pattern | asset? | clear any cue that only repeats the layer's shape |
| 13 | `UGeoBeamVFXComponent`, `UGeoIndicatorComponent`, `UGeoEffectZone`, `UZoneHazardPattern` | — | already zone layer | done |
| 14 | Niagara: teleporter portal, projectile bullets / buff trails, death debris, deploy satellites, `GeoFXComponent` moments | — | not footprints | keep |

## Code
- [ ] `GeoIndicatorComponent.h/.cpp` — `FGeoIndicatorState` gains `Layer` (`EGeoZoneLayer`, default `Telegraph`) and `FillMode` (`EGeoFillMode`, default `FromCore`); `SpawnIndicator` applies them. Defaults keep every current caller identical (replicated struct: two bytes).
- [ ] `GeoZoneIndicatorCue.h/.cpp` — Blueprint-editable defaults on the cue Blueprint: `Shape` (`Round` | `Ray`; `Ray`: `RawMagnitude` = length, `Normal.Z` = half-width, direction = the cue's `Normal` yaw convention of `Ray` in `FGeoIndicatorState`), `Layer`, `FillMode`. Round / Telegraph / FromCore = today. Update the class comment.
- [ ] Blueprint-callable draw: `UGeoAbilitySystemLibrary::SpawnZoneShape(WorldContext, Shape, Size, Color, SecondaryColors, Duration, AttachParent, Layer, FillMode, ElapsedTime)` → returns `UGeoZoneVisualComponent*` (null on a dedicated server); builds an `FGeoIndicatorState` and calls `SpawnIndicator`, so the cue and a Blueprint share one path. `UGeoZoneIndicatorCue::OnExecute` calls it too (one code path).
- [ ] `GeoZoneVisualComponent.h` — optional, later: `BlueprintSpawnableComponent` + `BlueprintReadWrite` shape / colour / fill, so an actor Blueprint holds one directly. Not needed for the cue route; only when a Blueprint needs a persistent shape it moves itself.
- [ ] Candidate 4 (if its asset is a circle) — `GeoDeployTargetCue.h/.cpp`: a `UGeoZoneVisualComponent` default subobject (`Circle`, `Telegraph`, radius from the cue's `RawMagnitude`, colour from the cue's palette slot), shown in `OnActive`, hidden in `OnRemove`; the old visual in the cue Blueprint removed.
- [ ] Candidate 5 (if decided) — `GC_Marked` reparented to the zone cue; nothing in C++.
- [ ] Candidates 3, 6-8, 12 — rows only after the user decides them.

## Assets (one-off headless Python, not saved; read every value back)
- [ ] `GC_ZoneIndicator` — defaults unchanged (`Round`, `Telegraph`, `FromCore`); open once to confirm the compile after the cue's new properties.
- [ ] Per decided candidate: the cue Blueprint's parent / properties; cleared `InitCue` on `BP_TileCarveRayPattern` if it only repeated the layer.
- [ ] Removal of a cue Blueprint (only candidate 5's old material, candidate 3's cue): `grep -la <tag>` on `Content/` finds only its own Blueprint, then delete (`AI/Commands.md`, a scripted delete can leave the file); tag removed from `Config/Tags/GeoGameplayTags.ini` and its `DefaultEngine.ini:263` mention.

## Docs
- [ ] `Actor/Deployable/CLAUDE.md` "Zone through a cue": written (this change). `Pillar/`, `BombZone/`: written.
- [ ] `Abilities/Common/CLAUDE.md`: row for `GeoZoneIndicatorCue.h`: written.
- [ ] `AbilitySystem/Data/FX/CLAUDE.md` ("Cues"): the footprint rule above, once the Blueprint function exists.
- [ ] `AI/VFX.md` "Meaning Colours" (devastating wave line; the rule) and its `GC_Marked` row when candidate 5 lands; `AI/Materials.md` "Zone layer" users list; `GeoIndicatorComponent.h` comment; `Pattern.h` `InitCue` comment ("wind-up telegraph").

## Tests
- [ ] `GeoZoneLayerTests.cpp` — `SpawnZoneShape`: `Round` / `Ray`, `Ground` / `Telegraph` layer, `FillMode` land on the shape; null on a dedicated server world; gone after `Duration`.
- [ ] Zone cue: `OnExecute` with deployable-style params draws one circle of radius `RawMagnitude`, lifetime `Normal.X`, attached to `SourceObject`'s root (existing behaviour kept: a test pins it).
- [ ] Candidate 4: charging deploy shows one circle at the pending location, gone on release end / cancel.
- [ ] Tutorial bot: no room or step change, coverage table unchanged.

## Verify (`test-fixer` agent)
- [ ] `Tools\Run_Tests.bat GeoTrinity.ZoneLayer`, `GeoTrinity.Gameplay.Deployable`, `GeoTrinity.Pattern`.
- [ ] `Tools\Run_Tests.bat GeoTrinity.Tutorial.FullRun` (gate).
- [ ] Headless load of `GC_ZoneIndicator` and every touched Blueprint: no load error.
