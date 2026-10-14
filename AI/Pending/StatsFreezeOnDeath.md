# StatsFreezeOnDeath — pending

Goal: a dead player's ø (average DPS / HPS) stops at their alive figure instead of shrinking while the fight goes on; live rate still falls to 0.
Docs written: `Source/GeoTrinity/Public/System/Stats/CLAUDE.md`.

## Tests
- [ ] `Private/Tests/GeoCombatStatsTests.cpp` — needs a minimal world + `AGeoPlayerState`: report damage, `ReportPlayerDied`, advance time → `FightDPS` unchanged, `LiveDPS` decays, `ResetStats` unfreezes. Run: `Tools\Run_Tests.bat GeoTrinity.CombatStats`.

## Verify
- [ ] Build Editor target (not built yet), then the test above.
