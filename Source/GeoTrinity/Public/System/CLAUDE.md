# System

World / game-instance subsystems and pooling.

| Folder / file | Holds |
|---|---|
| `Pooling/` | `GeoActorPoolingSubsystem`, `GeoPoolableInterface` |
| `Stats/` | `GeoCombatStatsSubsystem`, `GeoLeaderboardSave` |
| `GeoBulletSubsystem.h` | Maths-only pattern bullets: straight flight from payload start, same place on every machine at the same server time, server judges hits |
| `GeoSessionSubsystem.h` | GameInstance subsystem: direct-IP no-Steam host/join |

## Session
- `HostListen(MapPackagePath)` = `ServerTravel` to `<Map>?listen`; `JoinByAddress(Address)` = `ClientTravel` (port 7777); `GetLocalIPv4()` for display.
- Steam sessions stay in `UGeoGameInstance`; host and joiner must be in the same mode (both Steam or both not).
- Non-Steam net driver: `DefaultEngine.ini` falls back to `IpNetDriver`.
