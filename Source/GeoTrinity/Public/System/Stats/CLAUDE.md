# System/Stats

| File | Role |
|---|---|
| `GeoCombatStatsSubsystem.h` | Per-player DPS/HPS/damage-received, fixed-size state |
| `GeoLeaderboardSave.h` | Fight records on this machine: each player's best attempt per boss and difficulty (name, class, figures), build wipe |

## Combat stats
- Rates exponentially smoothed (`SmoothingWindowSeconds = 3`); `FightDPS` / `FightHPS` = totals / the player's own active time (`CombatStartTime` → their death, or now while alive), clamped to ≥ 1 s.
- Death stops the player's ø: `FActorCombatStats::DeathTime` set by `AGeoGameState::NotifyPlayerDied` (in-fight deaths only) → `ReportPlayerDied`; `PushPlayerStats` divides their totals by `DeathTime - CombatStartTime`, so the meter keeps the alive averages instead of ones diluted by the fight going on. The live rate decays to 0 as usual; peak and totals are untouched.
- `DeathTime` lives in the per-fight entry: cleared by `ResetStats()` (new fight).
- MAX columns (`FBurstTracker`): biggest total inside one tumbling 0.5 s window = biggest single spell.
- Server-only `OnWorldBeginPlay` subscribes to `AGeoGameState::OnMatchStateChanged`: entering InProgress `ResetStats()`; leaving empties `StatsPerActor` without pushing zeros (final values stay displayed).
- `Report*` decays rates, folds the event, pushes stats; first event outside a match opens a new session.
- Reports come from `UGeoAttributeSetBase::PostGameplayEffectExecute()`; `ComputePlayerStats` from `AGeoPlayerController::Tick`.

## Leaderboard
Arena hands each finished attempt to `UGeoLeaderboardSave::Record`; shown by `UGeoLeaderboardWidget`.
- Every machine of the fight records the attempt into its own file, so the file holds only attempts its owner was in.
- **One best attempt per player, per boss and difficulty.** Identity = `FGeoLeaderboardPlayer::PlayerName`. An attempt is kept while it is the best (`FGeoAttemptEntry::operator<`: least boss health left, then faster) of at least one of its players; otherwise dropped.
  - Same group again, faster: the new attempt replaces the old one for everyone in it — one row.
  - A player alone or in another group beats it: it becomes their best only; teammates who did not beat it keep the old row, so each player appears once and a group of best-run partners stays one row.
  - Ties keep the earlier entry (also makes a PIE re-record of the same attempt a no-op).
- `AddAttempt(Entry)` = add + `Prune()`, pure (no disk), so tests drive it. `Prune()` also runs in `Load()`, collapsing a file saved before this rule.
- **Build wipe.** `UGameDataSettings::bWipeLeaderboardOnNewBuild` (`DefaultGame.ini`): while true, `Load()` empties a file whose stored `BuildStamp` differs from the running build's, then stores the current stamp, so each new build wipes once and relaunching it keeps its runs. Stamp = modified time of the running executable (`ApplyBuildWipe(bWipe, CurrentStamp)`, pure). Set it true for a build that must start clean; scaffolding until the leaderboard is hosted on a server.
