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

Key flow: `ClientInitialize` → `InitOverlay()` (creates HUD), `OnPlayerPawnSet` delegate → `InitGAS()` on the character.

## `GeoGameState` — Boss fight lifecycle

Drives boss fight via UE's `HandleMatchHasStarted` / `HandleMatchIsWaitingToStart` / `HandleMatchHasEnded` hooks.

Key design points:
- **No `ArenaBarrier` property** — `GetArenaBarrier()` finds the actor via `UGameplayStatics::GetActorOfClass` at call time; set the actor in the level, not as a property reference.
- `SpawnEnemies()` spawns both `BossToSpawn` and `DummyToSpawn` on match restart; `HandleMatchIsWaitingToStart` destroys any existing boss/dummy first then re-spawns both.
- `InitBoss()` — shows HUD health bar locally, sends aggro StateTree event, closes barrier, starts `CommitFightTime` timer, counts live players.
- `NotifyPlayerDiedInFight()` — teleports the dead player to entrance, decrements `PlayersAliveInFight`; when 0, calls `Boss->ResetForNewAttempt()` and transitions to `WaitingToStart`.
- `CommitFightTime` — seconds from arena close to fight commit (players teleported to fight location); read by `AGeoArenaBarrier::TickLerp` as the lerp duration.
- `FightZoneTagName` / `EntranceZoneTagName` — tag names of exemption volumes; players already inside are skipped during teleport.
