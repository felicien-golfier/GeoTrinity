# GameClasses/Match

| File | Role |
|---|---|
| `GeoGameMode.h` | Rules, class assignment; match start via `StartMatch()` |
| `GeoGameState.h` | Match state (`WaitingToStart` / `InProgress`) + player death policy; no arena pointer |
| `GeoPlayerState.h` | Hosts ASC + `CharacterAttributeSet` + gem attributes for playable characters |

## GameState
- Owns only `MatchState` and death policy; everything room-shaped is on `AGeoArena`.
- Match start: `AGeoEnemyAIController::TriggerAggro` → `Arena->StartFight()` then `GameMode->StartMatch()`; arena `IsBoss()` gates it.
- `HandleMatchHasStarted()` (server) snapshots `FightPlayers` (weak ptrs, everyone alive).
- `OnMatchStateChanged` (`MatchState`, `PreviousMatchState`): broadcast on every machine and on authority; subscribe here (arena `EndFight`, `UGeoCombatStatsSubsystem`). On leaving InProgress clears `RespawnTimer` + `RevivePlayers()`.
- Death: `APlayableCharacter::DeathLogic()` → `NotifyPlayerDied` (disconnect via `GeoGameMode::Logout` → `Death()`).
  - Out of fight: revive on the spot.
  - In fight: down until `AreFightPlayersDead()`; then `OnWipe(DeathTime)`, `RespawnTimer` (lets fall-death teleport land first) → `RespawnGroup()`.
- `RespawnGroup()`: teleport to `TargetPoint.Entrance` for `CheckpointTag`, revive all, `RequestWaitingToStart()`.
- `CheckpointTag` / `SetCheckpointTag()`: set by the fighting arena in `StartFight()`; never replicates.
- `RequestWaitingToStart()` is the only way to stand a match down; never leave `WaitingPostMatch`.
- `CommitFightTime` / `DeathTime`: timing constants read by arena and barrier.

## PlayerState
- ASC survives respawn (standard GAS multiplayer).
- `PlayerClass` replicated → `OnRep_PlayerClass` → `ApplyClassDataToPawn()`; `TeamId` read by `GetGenericTeamId()`.
- Flow: `ClientInitialize` → `InitOverlay()`; `OnPlayerPawnSet` → `ApplyClassDataToPawn()` (all pawns); local player also `InitGAS()`. Early-out when `PlayerClass == None`.
- Replicated combat stats: smoothed DPS/HPS, `MaxBurstDamage` / `MaxBurstHealing`, fight averages, totals.
- Holds `UGeoGemComponent` (`Gem/CLAUDE.md`).
