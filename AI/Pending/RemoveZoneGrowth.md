# RemoveZoneGrowth — pending

Goal: zones no longer grow: `GrowToSize` / `GrowSeconds` and the growth machinery are removed; a zone is its `Size`.
Priority: 3
Docs written: none yet (workflow steps 2-3 when picked up). Owner docs: `Source/GeoTrinity/Public/Actor/Deployable/CLAUDE.md` ("Data and fields"), `Source/GeoTrinity/Public/Encounter/Action/CLAUDE.md`, `Source/GeoTrinity/Public/Characters/CLAUDE.md` (dummy zone), `AI/Tutorial/Steps.md` (steps 7, 10), `AI/Python/Tutorial/CLAUDE.md`, `Source/GeoTrinityEditor/Private/Tests/Tutorial/CLAUDE.md` (bot coverage).

## Discuss first (step 2)
- [ ] What replaces the growth where it is used: Zones room Damage zone (`ColorZoneMaxRadius` 1350 over `ColorZoneGrowSeconds` 8; Shield / Heal wait for it) and the Miniboss dummy zone (`DummyZoneMaxRadius` 700 over `DummyZoneGrowthTime` 12). Fixed at the final size, or another mechanic.
- [ ] Overlaps `ZonesRoomDamageZone.md` and `TutorialMinibossRework.md` (both P1): fold into them if picked up together.

## Code (to implement)
- [ ] `Actor/Deployable/GeoDeployableBase.h` — drop `FDeployableDataParams::GrowToSize`, `GrowSeconds`.
- [ ] `Actor/Deployable/Zones/GeoEffectZone.h/.cpp` — drop `GrowthStartServerTime` (+ its replication), `GrowthAlpha`, growth `Tick`; `GetRadius()` = `Data.Params.Size` (or callers read `Size` directly); class comment.
- [ ] `GeoTrinityEditor/Private/Tests/Gameplay/GeoEncounterTests.cpp` — delete `GeoTrinity.Deployable.GrowingZone`; `Tests/Gameplay/Fake/GeoTestEncounter.h` — drop `ZoneParams.GrowToSize` / `GrowSeconds`.
- [ ] `GeoTrinityEditor/Private/Tests/Tutorial/GeoTutorialRoomStages.cpp` — Zones stage waits on `ColorZoneMaxRadius` growth (lines ~72, ~967-1000): rework to the new layout.

## Scripts
- [ ] `AI/Python/Tutorial/Level/tutorial_encounters.py` — drop `GrowToSize` / `GrowSeconds` sets, `COLOR_ZONE_GROW_SECONDS` delay before Shield / Heal.
- [ ] `AI/Python/Tutorial/Level/tutorial_miniboss.py` — drop dummy `GrowToSize` / `GrowSeconds`; docstring rows.
- [ ] `AI/Python/Tutorial/Level/tutorial_assets.py` — `CT_Tutorial` rows `DummyZoneGrowthTime`, `DummyZoneMaxRadius`, `ColorZoneGrowSeconds`, `ColorZoneMaxRadius`: remove or repurpose as `Size`.
- [ ] `AI/Python/Tutorial/Runtime/pie_tutorial_jump.py` — `GROW_WAIT`.
- [ ] Re-run the tutorial scripts (`encounters_diff.py` first) so the map / `CT_Tutorial` match.

## Editor / Blueprint
- [ ] Any other asset setting `GrowToSize` (dummy BP, encounter data): find before the property goes, or it is silently lost.

## Verify
- [ ] Build; `GeoTrinity.Tutorial.` full set green (Encounter / tutorial change).
