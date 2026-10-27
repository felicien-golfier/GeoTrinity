# System

World / game-instance subsystems and pooling.

| Folder / file | Holds |
|---|---|
| `Pooling/` | `GeoActorPoolingSubsystem`, `GeoPoolableInterface` |
| `Stats/` | `GeoCombatStatsSubsystem`, `GeoLeaderboardSave` |
| `GeoBulletSubsystem.h` | Maths-only pattern bullets: straight flight from payload start, same place on every machine at the same server time, server judges hits |
| `GeoSessionSubsystem.h` | GameInstance subsystem: direct-IP no-Steam host/join |

## Session
- `HostListen(MapPackagePath, Portal = "")` = absolute `ServerTravel` to `<Map>?listen[#Portal]` (a relative one keeps the current portal when the URL has none); the engine spawns the host at the `PlayerStart` whose `PlayerStartTag` = `Portal`. `JoinByAddress(Address)` = `ClientTravel` (port 7777); `GetLocalIPv4()` for display.
- The tutorial map is a solo listen server (`HostListen(TutorialMap)`: encounters need `GeoLib::IsServer`); leaving it is `HostListen(TrainingRoomMap, "TrainingRoom")` (`AI/Tutorial/Flow.md`).
- Steam sessions stay in `UGeoGameInstance`; host and joiner must be in the same mode (both Steam or both not).
- Non-Steam net driver: `DefaultEngine.ini` falls back to `IpNetDriver`.
