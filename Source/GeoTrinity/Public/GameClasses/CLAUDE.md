# GameClasses

Core Unreal game framework classes.

## Files
| File | Role |
|---|---|
| `GeoGameMode.h` | Game rules, player class assignment; `Tick` override |
| `GeoGameState.h` | Replicated game state — enemy lifecycle, match transitions, arena barrier |
| `GeoGameInstance.h` | Persistent across levels; `LeaveSessionAndReturnToMenu()` — destroys the Steam session (if any) then opens `MainMenuMap`, used by the pause menu's "Return to Main Menu" |
| `GeoPlayerController.h` | Player controller; owns the pause menu widget (`PauseMenuWidgetClass`), toggled via `ToggleMenuAction`/`TogglePauseMenu()`/`ClosePauseMenu()` bound in `SetupInputComponent()`. While the menu is open, `SetMenuInputMappingActive` swaps the gameplay `InputMapping` for `MenuInputMapping` (`IMC_Menu`: only `IA_ToggleMenu`) so abilities can't fire behind the menu, flushes pressed keys, and focuses the menu widget for gamepad navigation |
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
- **The encounter lives on `AGeoArena`, not here.** The GameState owns *one match at a time* (UE's `MatchState` is global) but knows nothing about a particular boss, barrier or room: it reads all of that off the replicated `ActiveArena` pointer. A level holds one `AGeoArena` per encounter (main arena, hex arena, …). It is replicated because clients need it to resolve the boss health bar and the camera bounds.
- **`ActiveArena` is "where the players are", not "which fight is running" — it is never null on the server and is never cleared.** It is the checkpoint: a wipe sends everyone back to *that* arena's `TargetPoint.Entrance`, not to the hub. It starts as the arena whose `ArenaTag` matches `DefaultArenaTag` (the hub — an `AGeoDummyArena` in `DraftMap`) and moves whenever the players do: `AGeoTeleporter` (the pad's `ArenaTag` is the room on the far side) and `AGeoEnemyAIController::TriggerAggro` (the aggroed pawn's `Owner` *is* its arena). Both go through `SetActiveArena()`, which is server-only and fires `OnActiveArenaChanged` on every machine (manually on the authority, from `OnRep_ActiveArena` on clients) — that delegate is how the camera reframes, so anything else that should follow the players between rooms belongs on it rather than on a new match-state hook. Whether a fight is live is `MatchState`, never this pointer. The hub is claimed in `HandleMatchIsWaitingToStart` by a world scan, **not** in anyone's `BeginPlay`: the level's first `WaitingToStart` is set from `AGameMode::StartPlay` *before* `NotifyBeginPlay()`, so no actor has begun play yet when the entrance teleport needs the tag. Level actors already exist at that point, so the scan finds the arena; the boss reset is in the `else` branch of the same test, because at that first call the freshly-scanned arena's boss has not spawned yet and `ResetForNewAttempt()` would dereference a controller that does not exist.
- **No boss spawning here** — each arena spawns its own in `BeginPlay`, so every boss in the level exists to be aggroed. There is no `GetBossEnemy()` world scan and no `IsBoss()` class compare any more; `AGeoArena::IsBoss` is a pointer compare against the boss it spawned. It is also the single gate on this whole class: `TriggerAggro` is the only writer of `ActiveArena`, so an arena whose `IsBoss` returns false (`AGeoDummyArena`) never starts a match and never reaches any of the boss-fight code below.
- `HandleMatchIsWaitingToStart()` — resets the active arena's boss (or, on the level's first call, claims the `DefaultArenaTag` arena as `ActiveArena`), then teleports players to that arena's `TargetPoint.Entrance` points.
- `GetActiveArenaTag()` — `ActiveArena->ArenaTag`, falling back to `DefaultArenaTag` only on a client whose `ActiveArena` has not replicated yet. This is the single answer to "which arena are we in", used by the entrance/fight-location teleports, the camera, and `AGeoDeployableBase`'s push-out. `TeleportPlayersTo` takes only a `TargetPoint.*` purpose and resolves the arena through it.
- `StartBossFight(Boss)` — shows HUD health bar locally, sends aggro StateTree event, calls `ActiveArena->StartFight()`, starts `CommitFightTime` timer, snapshots live players.
- `StopBossFight()` — hides HUD health bar locally, calls `ActiveArena->EndFight()` (server), revives all players (server). Called from `HandleMatchHasEnded` and `OnRep_MatchState` whenever the match leaves InProgress, so it covers both boss defeat and group wipe. It **no longer destroys the boss** — the arena keeps it and `ResetBoss()` resets it in place; only a self-destructing defeated boss gets respawned.
- `RevivePlayers()` — iterates all player controllers and calls `Revive()` on each pawn.
- Player wipe tracking — `InitBossFight` snapshots the live `APlayableCharacter` pawns into `PlayersInFight` (a `TArray`), instead of a bare counter. `NotifyPlayerDied(player)` (called from `APlayableCharacter::Death()`): outside `InProgress` (e.g. during the fight-commit transition) it just revives that one player; during the fight it calls `HandlePotentialWipe()`. `HandlePotentialWipe()` checks `AreAllPlayersDead()` — true when every tracked player is dead (`IsDead()`) or no longer valid — and on a full wipe resets the boss and, after `DeathTime` seconds, calls `GeoGameMode::RequestWaitingToStart()`. `NotifyPlayerLeft()` (called from `AGeoGameMode::Logout`) removes the leaver from `PlayersInFight` before re-checking, so a disconnect can't keep the fight alive. Dead player's corpse stays in place; entrance teleport happens in `HandleMatchIsWaitingToStart`.
- Loot shower — `NotifyBossDefeated()` captures `LootOrigin` (boss location, before StopBossFight destroys it) then `Loot()` starts a looping `LootTimer` calling `SpawnLootBurst()`: each burst spawns `LootPickupsPerBurst` `AGeoBuffPickup`s at `LootOrigin`, launched to random points within `LootMaxRadius` (uniform disc). Config is read from the reload ability CDO found by class in `AbilityInfo` (`BuffPickupClass` is public on `UGeoReloadAbility` for this); Owner/Instigator = first live player pawn (pickup needs an ASC source + Friendly attitude); that player's deployable manager gets `SetDeployableInfinitCount(BuffPickupClass)` for the shower (tracked in `LootBoostedManagers`, restored via `RemoveDeployableSlot` when the arena resets). Stops in `HandleMatchIsWaitingToStart()`. Tunables: `LootSpawnInterval`, `LootPickupsPerBurst`, `LootMaxRadius`.
- `SetActiveArena(Arena)` / `FindArena(ArenaTag)` — the only write path for `ActiveArena`, and the tag→arena lookup its callers need. `SetActiveArena` ensures on the server and on a null arena; `FindArena` is a plain query returning null.
- `CommitFightTime` — seconds from arena close to fight commit (players teleported to the arena's `TargetPoint.FightLocation` points); read by `AGeoArenaBarrier::Tick` as the lerp duration. Global tuning, shared by every arena.
- `DefaultArenaTag` — arena the players **start the level in** (the hub), claimed as the first `ActiveArena`. Set in the editor on `BP_GeoGameState`; `Arena.Entrance` in `DraftMap`. It is not a "between fights" state — once a boss arena is entered, that arena stays active.
- `DeathTime` — delay (seconds) after a full wipe before transitioning back to WaitingToStart.
- `FightZoneTagName` / `EntranceZoneTagName` — tag names of exemption volumes; players already inside are skipped during teleport.
- There is no fight-commit delegate. `CommitFightDelegate` existed only so the camera could reframe at commit; the camera now follows `OnActiveArenaChanged` instead, so it was deleted rather than left unsubscribed. Note it could never have carried client-side listeners anyway — the commit timer runs on the authority only. Anything that must react to commit on every machine has to hang off replicated state.
- `OnMatchStateChanged` (`FMatchStateChanged`, two params: `MatchState`, `PreviousMatchState`) — broadcast on every match state transition; replaces the old per-state `MatchIsWaitingToStartDelegate`. Subscribe here instead of overriding `OnRep_MatchState` on clients.
