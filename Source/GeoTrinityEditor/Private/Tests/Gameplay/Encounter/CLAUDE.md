# Tests/Gameplay/Encounter

Encounter tests on the fake trigger / action / encounter / HUD (`Fake/GeoTestEncounter.h`); paths `GeoTrinity.Encounter.<Case>`.

| File | Covers |
|---|---|
| `GeoEncounterFlowTests.cpp` | Engine: step flow, condition, counter, repeat, latent action, busy rule, step-after, reentrant, restart (also while exiting / running), event bus, server only, EndPlay, validation, shared caster, session end, enter delay, kill on step exit |
| `GeoEncounterConditionTests.cpp` | Action condition (skip, rule payload, validation, no skip replay), step visit (rewind, per step, reset) |
| `GeoEncounterTriggerTests.cpp` | Triggers: health threshold (+ class, spawned same step, ignores dead, fired on arm), actor died, payload actor destroyed, ability activated, player idle, watched tag |
| `GeoEncounterAbilityTests.cpp` | `ActivateAbility` (+ refused, cancel, pattern data), ability room, ability page |
| `GeoEncounterSpawnTests.cpp` | `SpawnEnemies` (+ respawn, held), `SpawnZone` (+ held), arena spawn add reset |
| `GeoEncounterJumpTests.cpp` | Jump skip order / portal / lock, arrival abort / own rule / cancel undoes / skip replay, teleporter open, movement lock (+ restart) |
| `GeoEncounterCatTests.cpp` | Cat line, timed line, line from file, Cat actions |
| `GeoEncounterStartGateTests.cpp` | Start gate: a BeginPlay start waits for the login, then the possession; none after `EndPlay` |
| `GeoEncounterTestHelpers.h` | `FEncounterActions` alias and the shared helpers `PrepareEnemyWorld`, `SpawnCasterArena`, `GrantShared`, `NewActivateAction` |
