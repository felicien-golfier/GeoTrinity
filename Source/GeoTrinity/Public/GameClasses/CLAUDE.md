# GameClasses

Core Unreal game framework classes.

## Files
| File | Role |
|---|---|
| `GeoGameMode.h` | Game rules, player class assignment; `Tick` override |
| `GeoGameState.h` | Replicated match lifecycle + player death policy; holds no arena pointer |
| `GeoGameInstance.h` | Persistent across levels; `LeaveSessionAndReturnToMenu()` destroys Steam session then opens `MainMenuMap` |
| `GeoPlayerController.h` | Owns pause menu widget; toggled via `ToggleMenuAction`; swaps gameplay `InputMapping` for `MenuInputMapping` while open so abilities can't fire behind the menu |
| `GeoPlayerState.h` | **Hosts ASC + attribute sets for playable characters** |
| `GeoMainMenuGameMode.h` | Menu level GameMode; no pawn |
| `GeoMainMenuPlayerController.h` | Menu controller; creates `UGeoMainMenuWidget` on `BeginPlay` |

## `GeoPlayerState` — Critical for GAS
`UGeoAbilitySystemComponent`/`UCharacterAttributeSet` live here, not on the character (standard GAS multiplayer pattern — ASC survives respawn).
- `PlayerClass` (replicated) — `OnRep_PlayerClass` triggers class-switch visuals.
- `TeamId` — canonical team; `GetGenericTeamId()` reads it directly so attitude queries resolve even when the pawn is momentarily absent.
- Combat stats (replicated): smoothed DPS/HPS, best/fight-average DPS/HPS, totals.

Flow: `ClientInitialize` → `InitOverlay()`; `OnPlayerPawnSet` → `ApplyClassDataToPawn()` (runs for ALL pawns, covers remote players on dedicated servers); local-player path also runs `InitGAS()`. `OnRep_PlayerClass` also calls `ApplyClassDataToPawn()` so both race orderings are covered. Early-outs when `PlayerClass == None`.

## `GeoGameState` — match lifecycle + death policy
Owns **only** `MatchState` (`WaitingToStart`/`InProgress`) and the death policy — no arena/boss/barrier pointer. Everything room-shaped lives on `AGeoArena`, which drives itself off `OnMatchStateChanged`. The only thing a death needs from the encounter is `CheckpointTag`, a plain tag the aggroed arena registers.

Key points:
- No replicated arena reference at all. "Where are the players" is answered by `AGeoCameraVolume`; "which fight is live" by whichever arena has `bFighting` set. Match is started externally: `AGeoEnemyAIController::TriggerAggro` calls `Arena->StartFight()` then `GameMode->StartMatch()`.
- `AGeoArena::IsBoss` gates whether a match starts, but that check lives in `TriggerAggro` — an arena with `IsBoss==false` (`AGeoDummyArena`) never triggers fight machinery.
- `HandleMatchHasStarted()` (server): snapshots `FightPlayers` = everyone alive as the fight begins. Boss bar/barrier/commit stay the arena's.
- `OnRep_MatchState()`: on leaving `InProgress`, clears `RespawnTimer` + `RevivePlayers()`, then broadcasts `OnMatchStateChanged` on every machine (fires server-side too, since `SetMatchState` calls it on authority). Arena's `EndFight` hangs off this broadcast.
- **Death is a single policy, no arena consulted.** `APlayableCharacter::DeathLogic()` → `NotifyPlayerDied`; disconnect reaches the same path via `GeoGameMode::Logout` → `Death()`. Out of a fight: revive on the spot. In a fight: stays down until `FightPlayers` (weak-ptr snapshot) are all down (`AreFightPlayersDead()`) — late joiners can't block/trigger a wipe, stale/leaver entries just stop counting. On wipe, broadcasts `OnWipe(DeathTime)`; the arena cancels its pending commit and opens the barrier, fully open by respawn. Wipe uses a timer (`RespawnTimer`), not immediate — lets the hex arena's fall-death teleport land before `RespawnGroup` moves everyone.
- `RespawnGroup()` — wipe timer callback: teleports group to `TargetPoint.Entrance` for `CheckpointTag` (always, no exempt zone), revives everyone, `RequestWaitingToStart()`. Never touches an arena.
- `CheckpointTag`/`SetCheckpointTag()` — registered by the fighting arena in its `StartFight()`; read only during respawn, never replicates.
- `RequestWaitingToStart()` — the only way to stand a match down. **Never leaves it at `WaitingPostMatch`**: `AGameMode::StartMatch()` early-outs when `HasMatchStarted()` is true (true in `WaitingPostMatch`), which would silently block all future `TriggerAggro`.
- `CommitFightTime`/`DeathTime` — plain timing constants read by arena/barrier; kept here because multiple actors read them, not because of an arena reference.
- `OnMatchStateChanged` (`FMatchStateChanged`: `MatchState`, `PreviousMatchState`) — the seam `AGeoArena::EndFight` and `UGeoCombatStatsSubsystem` subscribe to. Subscribe here rather than overriding `OnRep_MatchState` on clients.
