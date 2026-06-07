# GameClasses

Core Unreal game framework classes.

## Files
| File | Role |
|---|---|
| `GeoGameMode.h` | Game rules, player class assignment; `Tick` override |
| `GeoGameState.h` | Replicated game state — enemy lifecycle, match transitions, arena barrier |
| `GeoGameInstance.h` | Persistent across levels |
| `GeoPlayerController.h` | Player controller |
| `GeoPlayerState.h` | **Hosts ASC + attribute sets for playable characters** |

## `GeoPlayerState` — Critical for GAS
The `UGeoAbilitySystemComponent` and `UCharacterAttributeSet` live on `AGeoPlayerState`, not on the character. This is the standard GAS multiplayer pattern — ASC survives character respawn.

Key fields:
- `UGeoAbilitySystemComponent* AbilitySystemComponent`
- `UCharacterAttributeSet* CharacterAttributeSet`
- `EPlayerClass PlayerClass` — replicated, `OnRep_PlayerClass` triggers class switch visuals
- Rolling combat stats (replicated): `DebugDPS`, `DebugHPS`, `DebugRecv`, `TotalDamageDealt`, `TotalHealingDealt`, `TotalDamageReceived`

Key flow: `ClientInitialize` → `InitOverlay()` (creates HUD), `OnPlayerPawnSet` delegate → `InitGAS()` on the character + `ApplyClassDataToPawn()` (guards against the race where `OnRep_PlayerClass` fires before the pawn exists).
`OnRep_PlayerClass` also calls `ApplyClassDataToPawn()` so both replication orders are covered.

## `GeoGameState` — Boss fight lifecycle

Drives boss fight via UE's `HandleMatchHasStarted` / `HandleMatchIsWaitingToStart` / `HandleMatchHasEnded` hooks.

Key design points:
- **No `ArenaBarrier` property** — `GetArenaBarrier()` finds the actor via `UGameplayStatics::GetActorOfClass` at call time; set the actor in the level, not as a property reference.
- `HandleMatchIsWaitingToStart()` — teleports players to entrance, then calls `SpawnEnemies()` which only spawns if no alive boss/dummy exists (idempotent).
- `InitBossFight(Boss)` — shows HUD health bar locally, sends aggro StateTree event, closes barrier, starts `CommitFightTime` timer, counts live players.
- `StopBossFight()` — destroys the boss, hides HUD health bar locally, opens the barrier (server), revives all players (server). Called from `HandleMatchHasEnded` and `OnRep_MatchState` (client path when leaving InProgress).
- `RevivePlayers()` — iterates all player controllers and calls `Revive()` on each pawn.
- `NotifyPlayerDiedInFight()` — decrements `PlayersAliveInFight`; when 0, waits `DeathTime` seconds then calls `GeoGameMode::RequestWaitingToStart()`. Dead player's corpse stays in place; entrance teleport happens in `HandleMatchIsWaitingToStart`.
- `CommitFightTime` — seconds from arena close to fight commit (players teleported to fight location); read by `AGeoArenaBarrier::TickLerp` as the lerp duration.
- `DeathTime` — delay (seconds) after a full wipe before transitioning back to WaitingToStart.
- `FightZoneTagName` / `EntranceZoneTagName` — tag names of exemption volumes; players already inside are skipped during teleport.
