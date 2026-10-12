# Tool/Net

Lag compensation and its debugging. Rule and usage: `AbilitySystem/Abilities/Pattern/CLAUDE.md`.

| File | Role |
|---|---|
| `GeoHazardJudge.h` | Server: judges each hostile at its own time over `[0, Duration]` at `SampleRate`; `FGeoLagCompensatedEvent` for one-moment events |
| `GeoNetcodeDebug.h` | `Geo.DebugNetcode` (bullets: `.Projectiles`) draws + visual-logs; calls cost nothing while the CVar is off |

- Colour code: cyan = where it is now, magenta = where a lagging view puts it in the past (rewind counterpart drawn +15 Y).
- `FGeoLagCompensatedEvent`: `bSeenThroughReplication` adds the player's half ping (capped); `JudgeActorsReachingEvent`, `IsOver` (`T + MaxLatencyCompensation`, doubled when replicated).
