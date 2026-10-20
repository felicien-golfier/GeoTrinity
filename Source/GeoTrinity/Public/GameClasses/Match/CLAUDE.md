# GameClasses/Match

| File | Role |
|---|---|
| `GeoGameMode.h` | Rules, class assignment; match start via `StartMatch()`; `bSoloOnly` |
| `GeoGameState.h` | Match state (`WaitingToStart` / `InProgress`) + player death policy; no arena pointer |
| `GeoPlayerState.h` | Hosts ASC + `CharacterAttributeSet` + gem attributes for playable characters |

## GameMode
- `bSoloOnly` (tutorial map's `BP_TutorialGameMode`): `InitNewPlayer` refuses a player when another player controller is already in the world (the new one is in the list by then; `AGameMode::GetNumPlayers` is no use here: it counts only players past `PostLogin`). Same gate for a remote joiner and a second local player: both pass through `Login`. The pause menu also pauses a solo world (`Player/CLAUDE.md`, "Solo pause").
- Player starts: a portal in the travel URL picks the start with that `PlayerStartTag` (engine `FindPlayerStart`); no portal → `ChoosePlayerStart` picks among untagged starts only. A tagged start (`TrainingRoom`, a tutorial room's jump start) is reached only by its portal.

## GameState
- Owns only `MatchState` and death policy; everything room-shaped is on `AGeoArena`.
- Match start: `AGeoEnemyAIController::TriggerAggro` → `Arena->StartFight()` then `GameMode->StartMatch()`; arena `IsBoss()` gates it.
- `HandleMatchHasStarted()` (server) snapshots `FightPlayers` (weak ptrs, everyone alive).
- `OnMatchStateChanged` (`MatchState`, `PreviousMatchState`): broadcast on every machine and on authority; subscribe here (arena `EndFight`, `UGeoCombatStatsSubsystem`). On leaving InProgress clears `RespawnTimer` + `RevivePlayers()`.
- Death: `APlayableCharacter::DeathLogic()` → `NotifyPlayerDied` (disconnect via `GeoGameMode::Logout` → `Death()`).
  - Out of fight: `RespawnPlayer` after `DeathTime`, at `TargetPoint.Entrance` of `CurrentArenaTag`.
  - In fight: down until `AreFightPlayersDead()`; then `OnWipe(DeathTime)`, `RespawnTimer` → `RespawnGroup()`.
- `OnCharacterDied(AGeoCharacter&)` (server, native): broadcast by `AGeoCharacter::Death()` for every character, player or enemy, falls and disconnects included; the one death hub for listeners that watch several characters (encounter triggers).
- `OnIncomingDamage` / `OnIncomingDamageShare` (server): the two stages a hit passes before shield and health, broadcast by `UGeoAttributeSetBase` (`AttributeSet/CLAUDE.md`); here so a listener binds once for every victim.
- `RespawnGroup()`: teleport to `TargetPoint.Entrance` for `CurrentArenaTag`, revive all, `RequestWaitingToStart()`.
- `CurrentArenaTag` / `SetCurrentArenaTag()`: set by `AGeoArenaVolume` on entry out of a fight and by the fighting arena in `StartFight()`; never replicates. Every room a player can die in needs a volume and `TargetPoint.Entrance` points for its `Arena.*` tag.
- `RequestWaitingToStart()` is the only way to stand a match down; never leave `WaitingPostMatch`.
- `CommitFightTime` / `DeathTime`: timing constants read by arena and barrier.

## PlayerState
- ASC survives respawn (standard GAS multiplayer).
- `PlayerClass` replicated → `OnRep_PlayerClass` → `ApplyClassDataToPawn()`; `TeamId` read by `GetGenericTeamId()`.
- Flow: `ClientInitialize` → `InitOverlay()`; `OnPlayerPawnSet` → `ApplyClassDataToPawn()` (all pawns); local player also `InitGAS()`. Early-out when `PlayerClass == None`.
- Replicated combat stats: smoothed DPS/HPS, `MaxBurstDamage` / `MaxBurstHealing`, fight averages, totals.
- Holds `UGeoGemComponent` (`Gem/CLAUDE.md`).
