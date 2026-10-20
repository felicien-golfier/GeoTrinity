# EncounterCodeCleanup — pending

Goal: less encounter / tutorial code: one way to watch characters, one way to spawn an enemy, one deployable spawn funnel, no unused actions, one gem trigger, and no step-ordering trap on spawned actors' health.
Docs written: `Encounter/Trigger/CLAUDE.md` (`FGeoActorFilter`, watching rules, `GeoGemProfileTrigger`, spawned-actor timing), `Encounter/Action/CLAUDE.md` (rows removed, `SpawnEnemy`, zone through the funnel, "no action until a step needs one"), `Encounter/CLAUDE.md` (no character references), `Characters/CLAUDE.md` ("Enemy setup": `SpawnEnemy`), `Actor/Arena/CLAUDE.md` (`SpawnAdd`), `AbilitySystem/Lib/CLAUDE.md` (self-owned funnel), `Actor/Deployable/CLAUDE.md` (lifecycle).
Follows `EncounterSpawnSetup.md` (code done there); can land before or after its editor items.
Grouping checked: the filter struct sits with the triggers that use it (`Encounter/Trigger/`); the gem trigger pair becomes one file.

## Left
- [ ] Re-run `AI/Python/Level/tutorial_encounters.py` (headless, editor closed; script already edited to `GeoActorFilter`): rebuilds the encounters' instanced triggers with the new properties. Old `WatchedClass` / `WatchedTag` values on the saved encounters are gone with the properties.
- [ ] Test `GeoGemProfileTrigger` (`Equipped` / `Crafted`): needs a local player with a gem profile (`UGeoGemSubsystem::GetFirstLocalProfile`), not available in the test world; no test written.

Done: filter, health trap, `SpawnEnemy` (takes `AGeoArena*` as well as `Owner`: the StateTree starts on possession and reads `Arena`), self-owned funnel, three actions deleted (no map or script held them), merged gem trigger, tests (28 Encounter, Arena, Deployable, Gem, Characters green).
