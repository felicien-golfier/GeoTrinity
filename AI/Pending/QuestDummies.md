# QuestDummies — pending

Goal: the step 3 quest dummies respawn only when they die (5 s after their own death, at their death spot, no timer on the card), and there are 9 in 3 groups of 3 so the Healer beam can touch only one group at a time.
Docs written: `AI/Tutorial/Steps.md` (step 3), `Encounter/Action/CLAUDE.md` (`GeoSpawnEnemiesAction`, `GeoQuestCardAction`), `GeoTrinityUI/.../HUD/Tutorial/CLAUDE.md` (quest card).
Design choice (user): beam limit = room layout only, no cap on the beam ability.

## Code (to implement)
- [ ] `Encounter/Action/GeoSpawnEnemiesAction.h/.cpp` — drop the shared refill tick; per spawned enemy bind its death, record its location, start its own `RespawnDelay` timer (world time), respawn there with the same `Setup`; `OnCancel` clears every timer and binding; no respawn for a cancel / wipe destroy
- [ ] `Encounter/Action/GeoQuestCardAction.h/.cpp` — `RespawnSeconds` → `TimeLimitSeconds` (default 1800)
- [ ] `HUD/Interface/GeoHUDInterface.h` (GeoTrinity) + `GeoTrinityUI` `AGeoHUD::ShowQuestCard(Title, Max, TimeLimitSeconds)` — the argument now means the quest time limit
- [ ] `HUD/Tutorial/GeoQuestCardWidget.h/.cpp` — countdown no longer loops: remaining = `TimeLimitSeconds` − (world time − `ShownTime`), shown `mm:ss`, clamped at 00:00
- [ ] Tests (`GeoEncounterTests.cpp`): `GeoTrinity.Encounter.SpawnEnemiesRespawn` — a killed one comes back after `RespawnDelay` at its death location; one kill never replaces the others; cancel leaves no timer and no new spawn

## Scripts
- [ ] `AI/Python/Level/tutorial_encounters.py` — `build_shoot_room`: 9 `TargetPoint.AddSpawn` points = 3 rays from the entry spot ≥ 60° apart, 3 points per ray (a group in a line); step 3 `b.spawn(dummy, 9, …)`, `b.card(SHOW, Title, Max, TimeLimitSeconds=1800)` (new `QUEST_TIME_LIMIT`); `QUEST_RESPAWN` stays only as the spawn action's `RespawnDelay`; Cat line "Ten Dummies are already here, and they respawn every 5 seconds" → nine, no timer wording; quip at 6 ("Don't mind the respawn") still fits. Run headless, editor closed

## Editor / Blueprint (by hand or script)
- [ ] `WBP_QuestCard` — keep the countdown text widget (now the 30 min limit)
- [ ] Artifact "Tutorial Board" page 6 (`AI/Artifacts.md`): respawn timer becomes a 30 min quest limit, "ten" becomes nine; user edits the artifact, Claude syncs back

## Verify
- [ ] Build Editor; `Tools\Run_Tests.bat GeoTrinity.Encounter`
- [ ] Layout: bridge read of `/Game/Maps/Tutorial`, 9 points in 3 groups; for each group, no other point inside the beam rectangle (`GeneralSpellDistance` long, widest `GetCurrentBeamHalfWidth`) from any spot on the group's ray
