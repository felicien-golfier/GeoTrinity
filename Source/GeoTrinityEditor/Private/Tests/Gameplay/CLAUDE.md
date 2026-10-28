# Tests/Gameplay

Automation tests of the `GeoTrinity` module. Fakes they spawn: `Fake/CLAUDE.md`.

| File | Covers |
|---|---|
| `GeoPatternTests.cpp` | Safe spot maths: `Behind` / `Lane` / `Disc` shadows, mask slots; salve aim per `EProjectileTarget` |
| `GeoGemTests.cpp` | Gem catalog, sockets, stats, CSV sync, deployable start health |
| `GeoCatLineBankTests.cpp` | Cat line bank: parse, bags, file order, live reload, missing file |
| `GeoAbilityReadyTests.cpp` | Ability ready state |
| `GeoIncomingDamageTests.cpp` | Incoming damage |
| `GeoCombatStatsTests.cpp` | Combat stats subsystem |
| `GeoMontageOwnershipTests.cpp` | Montage ownership |
| `GeoSoundConcurrencyTests.cpp` | Sound concurrency assets |
| `GeoTutorialTests.cpp` | Solo-only login, listen URL, locked / unlocked abilities |
| `GeoCatTests.cpp` | Cat walk, stop distance, hidden walk, replace, roam, montage order |
| `GeoEncounterTests.cpp` | Fake trigger / actions / encounter / HUD, readable zone, set-up summoned dummy |
| `GeoVoidDeathTests.cpp` | Void death with an enemy whose body stays |
| `GeoTurretTests.cpp` | Player turret waits for the owner's basic-hit target, idles when it is gone |

- Each test builds its own world with `CreateTestWorld` / `DestroyTestWorld` (`Fake/GeoTestWorld.h`); none touches a map asset.
