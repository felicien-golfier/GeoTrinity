# GameClasses

Core Unreal game framework classes.

## Files
| File | Role |
|---|---|
| `GeoGameMode.h` | Game rules, player class assignment; `Tick` override |
| `GeoGameState.h` | Replicated game state — enemy lifecycle, match transitions, arena barrier |
| `GeoGameInstance.h` | Persistent across levels; `LeaveSessionAndReturnToMenu()` — destroys the Steam session (if any) then opens `MainMenuMap`, used by the pause menu's "Return to Main Menu" |
| `GeoPlayerController.h` | Player controller; owns the pause menu widget (`PauseMenuWidgetClass`), toggled via `ToggleMenuAction`/`TogglePauseMenu()`/`ClosePauseMenu()` bound in `SetupInputComponent()` |
| `GeoPlayerState.h` | **Hosts ASC + attribute sets for playable characters** |
| `GeoMainMenuGameMode.h` | Menu level GameMode (`AGameModeBase`); no pawn; sets `PlayerControllerClass` to `AGeoMainMenuPlayerController` |
| `GeoMainMenuPlayerController.h` | Menu player controller; creates `UGeoMainMenuWidget` on `BeginPlay`, adds to viewport, sets UI-only input |

## `GeoPlayerState` — Critical for GAS
The `UGeoAbilitySystemComponent` and `UCharacterAttributeSet` live on `AGeoPlayerState`, not on the character. This is the standard GAS multiplayer pattern — ASC survives character respawn.

Key fields:
- `UGeoAbilitySystemComponent* AbilitySystemComponent`
- `UCharacterAttributeSet* CharacterAttributeSet`
- `EPlayerClass PlayerClass` — replicated, `OnRep_PlayerClass` triggers class switch visuals
- `ETeam TeamId = ETeam::Player` — canonical team for this player; `GetGenericTeamId()` reads this directly so attitude queries on the PlayerState resolve even when the pawn is momentarily absent (respawn, possession order on server)
- Combat stats (replicated): smoothed `DebugDPS`/`DebugHPS`, `BestDPS`/`BestHPS`, whole-fight averages `FightDPS`/`FightHPS`, `TotalDamageDealt`, `TotalHealingDealt`, `TotalDamageReceived`

Key flow: `ClientInitialize` → `InitOverlay()` (creates HUD), `OnPlayerPawnSet` delegate → `ApplyClassDataToPawn()` (runs for ALL pawns — covers remote players' class visuals on dedicated servers), then local-player path → `InitGAS()` on the character.
`OnRep_PlayerClass` also calls `ApplyClassDataToPawn()` so both race orderings (pawn set before class, class set before pawn) are covered.
`ApplyClassDataToPawn()` early-outs when `PlayerClass == EPlayerClass::None` (server-side: class not yet assigned).

## `GeoGameState` — Boss fight lifecycle

Drives boss fight via UE's `HandleMatchHasStarted` / `HandleMatchIsWaitingToStart` / `HandleMatchHasEnded` hooks.

Key design points:
- **No `ArenaBarrier` property** — `GetArenaBarrier()` finds the actor via `UGameplayStatics::GetActorOfClass` at call time; set the actor in the level, not as a property reference.
- `HandleMatchIsWaitingToStart()` — teleports players to entrance, then calls `SpawnEnemies()` which only spawns if no alive boss/dummy exists (idempotent).
- `InitBossFight(Boss)` — shows HUD health bar locally, sends aggro StateTree event, closes barrier, starts `CommitFightTime` timer, counts live players.
- `StopBossFight()` — destroys the boss, hides HUD health bar locally, opens the barrier (server), revives all players (server). Called from `HandleMatchHasEnded` and `OnRep_MatchState` (client path when leaving InProgress).
- `RevivePlayers()` — iterates all player controllers and calls `Revive()` on each pawn.
- Player wipe tracking — `InitBossFight` snapshots the live `APlayableCharacter` pawns into `PlayersInFight` (a `TArray`), instead of a bare counter. `NotifyPlayerDied(player)` (called from `APlayableCharacter::Death()`): outside `InProgress` (e.g. during the fight-commit transition) it just revives that one player; during the fight it calls `HandlePotentialWipe()`. `HandlePotentialWipe()` checks `AreAllPlayersDead()` — true when every tracked player is dead (`IsDead()`) or no longer valid — and on a full wipe resets the boss and, after `DeathTime` seconds, calls `GeoGameMode::RequestWaitingToStart()`. `NotifyPlayerLeft()` (called from `AGeoGameMode::Logout`) removes the leaver from `PlayersInFight` before re-checking, so a disconnect can't keep the fight alive. Dead player's corpse stays in place; entrance teleport happens in `HandleMatchIsWaitingToStart`.
- Loot shower — `NotifyBossDefeated()` captures `LootOrigin` (boss location, before StopBossFight destroys it) then `Loot()` starts a looping `LootTimer` calling `SpawnLootBurst()`: each burst spawns `LootPickupsPerBurst` `AGeoBuffPickup`s at `LootOrigin`, launched to random points within `LootMaxRadius` (uniform disc). Config is read from the reload ability CDO found by class in `AbilityInfo` (`BuffPickupClass` is public on `UGeoReloadAbility` for this); Owner/Instigator = first live player pawn (pickup needs an ASC source + Friendly attitude); that player's deployable manager gets `SetDeployableInfinitCount(BuffPickupClass)` for the shower (tracked in `LootBoostedManagers`, restored via `RemoveDeployableSlot` when the arena resets). Stops in `HandleMatchIsWaitingToStart()`. Tunables: `LootSpawnInterval`, `LootPickupsPerBurst`, `LootMaxRadius`.
- `CommitFightTime` — seconds from arena close to fight commit (players teleported to fight location); read by `AGeoArenaBarrier::TickLerp` as the lerp duration.
- `DeathTime` — delay (seconds) after a full wipe before transitioning back to WaitingToStart.
- `FightZoneTagName` / `EntranceZoneTagName` — tag names of exemption volumes; players already inside are skipped during teleport.
- `CommitFightDelegate` — broadcast when the fight-commit timer fires (players teleported to fight zone).
- `OnMatchStateChanged` (`FMatchStateChanged`, two params: `MatchState`, `PreviousMatchState`) — broadcast on every match state transition; replaces the old per-state `MatchIsWaitingToStartDelegate`. Subscribe here instead of overriding `OnRep_MatchState` on clients.
