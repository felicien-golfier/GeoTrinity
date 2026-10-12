# System/Stats

| File | Role |
|---|---|
| `GeoCombatStatsSubsystem.h` | Per-player DPS/HPS/damage-received, fixed-size state |
| `GeoLeaderboardSave.h` | Fight records on this machine, one entry per finished attempt (name, class, figures) |

## Combat stats
- Rates exponentially smoothed (`SmoothingWindowSeconds = 3`); `FightDPS` / `FightHPS` = totals / time since `CombatStartTime`.
- MAX columns (`FBurstTracker`): biggest total inside one tumbling 0.5 s window = biggest single spell.
- Server-only `OnWorldBeginPlay` subscribes to `AGeoGameState::OnMatchStateChanged`: entering InProgress `ResetStats()`; leaving empties `StatsPerActor` without pushing zeros (final values stay displayed).
- `Report*` decays rates, folds the event, pushes stats; first event outside a match opens a new session.
- Reports come from `UGeoAttributeSetBase::PostGameplayEffectExecute()`; `ComputePlayerStats` from `AGeoPlayerController::Tick`.
- Leaderboard: arena hands each finished attempt to the save; shown by `UGeoLeaderboardWidget`.
