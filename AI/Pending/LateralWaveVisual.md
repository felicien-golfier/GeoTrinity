# LateralWaveVisual — pending

Goal: a sweeping beam's telegraph makes the beam's own move (tutorial waves: from the start edge along the travel), and the boss's lateral wave travels in one of the 4 basic directions picked at random.
Priority: 1
Docs written: `Source/GeoTrinity/Public/AbilitySystem/Abilities/Pattern/Hazard/CLAUDE.md` ("Beam" telegraph rule, "LateralWave"), `AI/Tutorial/Steps.md` (step 8).

## Design decisions (2026-10-30)
- Generic rule (in the Hazard doc): whole-map sweeping beam → telegraph does the same move; caster-tied / conditional beam (hex boss) → telegraph only shows the start; ambiguous beam or zone → ask the user.
- Random direction: boss firings only. Tutorial Zones room keeps its fixed yaw. Safe lanes always parallel to the travel, turning with it.

## Code (to implement)
Status 2026-10-30: code written, formatted, NOT built (editor open, Live Coding). Build `Tools\Build_Editor.bat`, run `GeoTrinity.Pattern.` + `GeoTrinity.Tutorial.` set + `FullRun`.
New: `AGeoArenaVolume::GetArenaBounds` (arena box from its volumes; add a line to `Actor/Arena/CLAUDE.md` Volume row when built), `FSTTask_FireAbility::PatternData`.
- [x] `LateralWavePattern.h/.cpp` — wind-up in `MoveBeamVfx`: rectangle from the start edge, `Length = TravelDistance * windup progress` along the travel (progress = 1 + SpentTime / wind-up length, clamped 0..1; check what the base `UBeamPattern` exposes for the wind-up length), `Fill` unchanged.
- [x] `FLateralWavePatternData` — add `bRandomDirection` (default false).
- [x] `LateralWavePattern` — `InitPattern`: when `bRandomDirection`, `FRandomStream(Payload.Seed)` → yaw 0/90/180/270, stored in a runtime member used by `GetForward()`; `TravelYaw` stays the fixed value.
- [x] Rolled wave: derive origin (arena edge opposite the travel), `TravelDistance`, `HalfWidth` from the caster's arena bounds along / across the roll (find the arena-extent API; `Actor/Arena/CLAUDE.md`). Fixed-yaw wave: values as given.
- [x] Safe spots: `GetSafeSpots()` already uses `GetForward()`; check `SafeSpotOffsets` stay relative to the origin for a rolled wave.
- [ ] Doc touch-up if built differs: Hazard `CLAUDE.md`.

## Decided
- The tutorial miniboss is the only firing with a random direction (for now); Zones room waves stay fixed.
- [x] Miniboss firing sets `bRandomDirection`: code done (`FSTTask_FireAbility::PatternData`); left: `tutorial_miniboss.py` sets `PatternData` = `FLateralWavePatternData(bRandomDirection=True)` on the Wave state's task (new tree and the existing-tree branch); its lanes come from the adds, so they follow the roll.
- [x] Tutorial bot: `Path.MinibossWaveLanes` (and any miniboss wave stage) must read the rolled direction (same seed roll) instead of assuming a fixed yaw; update its coverage table.

## Scripts
- [ ] `AI/Python/Tutorial/Level/tutorial_waves.py` / `tutorial_encounters.py` — nothing expected (fixed yaw kept); re-check `BP_LateralWave` defaults after the change.

## Tests
- [x] `GeoPatternTests.cpp` / `GeoTrinity.Pattern.LateralWaveRoll` — roll (lane parallelism holds by construction: one `GetForward()`; not tested) from a seed: same yaw for equal seeds, always one of 0/90/180/270, all four reachable over seeds; rolled lanes stay parallel to the roll; a fixed wave ignores the roll.
- [ ] Tutorial bot: wave stages (`GeoTutorialRoomStages.cpp`) unchanged in direction; run `GeoTrinity.Tutorial.` set + `FullRun` (CLAUDE.md gate).

## Verify
- [ ] Telegraph visibly grows along the travel, start edge to far edge, then the band follows (user check; no PIE unless asked).
