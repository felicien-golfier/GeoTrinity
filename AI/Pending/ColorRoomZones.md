# ColorRoomZones — pending

Goal: the Zones room teaches colors with a growing tiny-damage zone that stays, then a Shield (left) and a Heal (right) inside it, both to stand in once, then 2 s and everything stops.
Docs written: `AI/Tutorial/Steps.md` (step 7, Tuning), `Source/GeoTrinity/Public/Encounter/Action/CLAUDE.md` (GeoSpawnZoneAction), `Source/GeoTrinityEditor/Private/Tests/Tutorial/CLAUDE.md` (coverage).

## Code (to implement)
- [ ] No C++ expected: `GeoSpawnZoneAction.Params` already carries `GrowToSize` / `GrowSeconds`. Confirm in the build that a zone spawned this way grows (`GeoEffectZone::Tick`); if not, fix there.
- [ ] `Source/GeoTrinityEditor/Private/Tests/Tutorial/GeoTutorialRoomStages.cpp` — replace `AddZoneColorStage` x3 with `AddZoneColorsStages`: Damage zone found at the centre, its radius grows (> first radius) up to `ColorZoneMaxRadius`, the pawn hurt by it (invincibility off for this stage); Shield and Heal zones appear after the growth; pawn moved into each (`FindZoneAt`); step still running right after the second entry, zones all gone ~2 s later (`FindZoneAt` null for the three). `ZonesLearnedLine` kept for the completion line.

## Scripts
- [ ] `AI/Python/Tutorial/Level/tutorial_assets.py` — `CT_Tutorial` rows `ColorZoneDamage` (2), `ColorZoneGrowSeconds` (8), `ColorZoneMaxRadius` (1350: past the room's corner, half 700 x 1100); placeholders.
- [ ] `AI/Python/Tutorial/Level/tutorial_encounters.py` (run `encounters_diff.py` first, `AI/Python/Tutorial/CLAUDE.md` "Read the scene first"):
  - `ZONE_SPOTS` = `{"Damage": (0, 0), "Shield": (150, -550), "Heal": (150, 550)}` (X from room centre, Y); drop the Heal / Damage swap of the old spots; `ZONE_X` per spot.
  - Move `Tut_Zones_ZonePoint_Damage` to the room centre, `ZonePoint_Shield` to Y -550, `ZonePoint_Heal` to Y +550 (hand-edit ledger: report `kept by hand`).
  - Boxes `Tut_Zones_ZoneVolume_*`: only Shield and Heal; delete the Damage one and the old scene boxes.
  - `Builder.zone(color)`: Damage = `GrowToSize` / `GrowSeconds` from the CT rows, `Size` small (100), amount `ColorZoneDamage`; Shield / Heal keep `ZONE_RADIUS`.
  - `zones_steps`: replace the three `Zones_<Color>` steps with one `ZoneColors`. `OnEnter`: intro narration (reworked third line), Damage zone, `delay(GrowSeconds)`, Shield zone then `count("ShieldUp")`, Heal zone then `count("HealUp")`. Rules (`bRepeat` true, condition counter `ShieldUp` / `HealUp` >= 1): Shield box entered → `count("Shield")`, Heal box entered → `count("Heal")`; completing rule on counter `Stood` (each of the two adds one; `Stood` == 2): `[delay(2.0), say(colors learned)]`, `completes=True`.
  - Update the intro lines (damage first; ask: stand in the shield, then the heal).
  - Tag rule: `StepAfter` of `ZoneColors` = `LethalWave` (default next).
- [ ] Re-run order: `tutorial_assets.py` → `tutorial_encounters.py`; then `scene_dump.py` / `sync-tutorial-scripts` if the scene was hand-edited.

## Editor / Blueprint (by hand or script)
- [ ] Check the left / right on the room's camera: Shield on the screen's left, Heal on the right; swap the Y signs if reversed.

## Verify
- [ ] Build editor, run `GeoTrinity.Tutorial.FullRun` then the whole `GeoTrinity.Tutorial.` set (Encounter folder not touched, but the room's script changed).
- [ ] Open question: a player standing in a spot before its zone spawns must step out and back in (the volume trigger fires on arm, the counter condition ignores it). If that feels wrong in play, spawn Shield and Heal on the player's request instead.
