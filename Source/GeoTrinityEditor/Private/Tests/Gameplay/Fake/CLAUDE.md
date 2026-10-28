# Tests/Gameplay/Fake

Stand-ins the gameplay tests spawn. Every `UCLASS` is `NotPlaceable, Transient`: never referenced by an asset, a map or a config.

| File | Holds |
|---|---|
| `GeoTestWorld.h` | Listen-server test world: `CreateTestWorld` / `DestroyTestWorld` |
| `GeoTestDeployable.h` | Concrete `AGeoDeployableBase` + deployer pawn (`AI/Testing.md`, "Spawning a deployable in a test") |
| `GeoTestTurret.h` | Turret with default attributes and a public `FindBestTarget` |
| `GeoTestEncounter.h` | Fake trigger / actions / encounter / HUD |
| `GeoTestEnemy.h` | Enemy whose body stays after death |
| `GeoTestGemDrop.h` | Gem drop stand-in |
| `GeoTestPatternAbility.h` | Fake patterns + pattern abilities |

- Need another behaviour: extend a fake or subclass it; production code is never changed to suit a test.
