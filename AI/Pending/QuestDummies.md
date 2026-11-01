# QuestDummies — pending

Goal: the step 3 quest dummies respawn only when they die (5 s after their own death, at their death spot, no timer on the card), and there are 9 in 3 groups of 3 so the Healer beam can touch only one group at a time.
Priority: 2
Docs written: `AI/Tutorial/Steps.md` (step 3), `Encounter/Action/CLAUDE.md` (`GeoSpawnEnemiesAction`, `GeoQuestCardAction`), `GeoTrinityUI/.../HUD/Tutorial/CLAUDE.md` (quest card).
Done 2026-10-21: per-enemy respawn, `TimeLimitSeconds` countdown on world time, `GeoTrinity.Encounter.SpawnEnemiesRespawn`, 9 points in 3 rays 70° apart (a point of another ray is at least 329 cm off a ray, beam half width = half the capsule radius + 50 per absorbed zone), script applied (scene vs script `identical`), `WBP_QuestCard` keeps `TimerText`.

## Left (user)
- [ ] Artifact "Tutorial Board" page 6 (`AI/Artifacts.md`): respawn timer becomes a 30 min quest limit, "ten" becomes nine; the user edits the artifact, Claude syncs back

## Status 2026-11-01
1 unticked item(s) remain; they need the editor, Blueprint/PIE, a build or test run, or a user decision, so none can be verified from the repo alone.
