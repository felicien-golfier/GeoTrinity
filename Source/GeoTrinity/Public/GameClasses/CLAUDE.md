# GameClasses

Core Unreal game framework classes.

## Files
| File | Role |
|---|---|
| `GeoGameMode.h` | Game rules, player class assignment; `Tick` override |
| `GeoGameState.h` | Replicated game state — match lifecycle + player death policy; holds no arena pointer |
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

## `GeoGameState` — match lifecycle + death policy

Runs **two things and holds no arena pointer**: the match lifecycle (`MatchState`: `WaitingToStart` until a boss is aggroed, `InProgress` while a fight runs) and the player death policy. Everything room-shaped — boss, barrier, fight commit, loot, the boss bar — lives on `AGeoArena`, which drives *itself* off this class's `OnMatchStateChanged` (see `Actor/CLAUDE.md`). The one thing a death needs from the encounter arrives as a plain tag, `CheckpointTag`, that the aggroed arena registers.

Key design points:
- **The GameState knows no arena, boss, barrier or room.** It has no replicated properties of its own (no `ActiveArena` any more — that concept is gone entirely). "Where are the players" is answered without it: the camera by the `AGeoCameraVolume` you stand in, and "which fight is live" by whichever `AGeoArena` has its replicated `bFighting` set. The match is *started* from outside too: `AGeoEnemyAIController::TriggerAggro` calls `Arena->StartFight()` then `GameMode->StartMatch()` directly. So this class never reaches into an arena — the arrows all point inward (arena → subscribes to it) or are one-way tag writes (arena → `SetCheckpointTag`).
- **`AGeoArena::IsBoss` is the single gate on whether a match starts, but that gate lives in `TriggerAggro`, not here.** An arena whose `IsBoss` returns false (`AGeoDummyArena`) never gets `StartFight`/`StartMatch`, so `MatchState` never leaves `WaitingToStart` for it and none of the fight machinery runs.
- `HandleMatchHasStarted()` — **server only**: snapshots into `FightPlayers` every player alive as the fight begins. That is this class's whole reaction to a fight starting; the boss bar, barrier and commit are the arena's, driven by `bFighting`/`StartFight`. (Clients need nothing here — the bar comes off `OnRep_bFighting`.)
- `OnRep_MatchState()` — on **leaving** `InProgress` (server): clears `RespawnTimer` and `RevivePlayers()` (stands up anyone still down — the mid-fight casualties on a victory). Then broadcasts `OnMatchStateChanged` on **every** machine, which is what the arena hangs `EndFight` off. There is no `HandleMatchHasEnded`/`HandleMatchIsWaitingToStart` override: `AGameState::SetMatchState` calls `OnRep_MatchState()` on the authority too, so this fires server-side already, and the old boss-reset that lived in `HandleMatchIsWaitingToStart` moved into the arena's `EndFight`. The synchronous chain (boss death → `RequestWaitingToStart` → `OnRep_MatchState` → `OnMatchStateChanged` → arena `EndFight` → `ResetBoss`) runs in exactly the same stack the old code did, so it is not a new reentrancy.
- **Player death is the single policy — no arena is consulted.** `APlayableCharacter::DeathLogic()` calls `NotifyPlayerDied(*this)`, and a disconnect reaches the same path because `AGeoGameMode::Logout` calls `Death()` on the leaver's pawn (no second bookkeeping path). Exactly two cases: **out of a fight** (`!IsMatchInProgress()`) the player is revived on the spot — what the training dummy used to do in an override, now universal; **in a fight** the player stays down, and once every player alive when the fight began is down, the whole group respawns after `DeathTime`. That "who started the fight" set is `FightPlayers`, a `TWeakObjectPtr` snapshot captured in `HandleMatchHasStarted`: a wipe is "all of these are down", so a late joiner (not in the snapshot) can neither block a wipe nor trigger one, and a stale entry (a leaver's pawn destroyed) simply stops counting. `AreFightPlayersDead()` is that check. At wipe detection the GameState also broadcasts **`OnWipe(DeathTime)`** (server only, still no arena pointer — arenas subscribe like they do to `OnMatchStateChanged`); the fighting arena reacts by cancelling its pending commit and opening the barrier, so it spends the `DeathTime` window lerping open and is fully open at respawn. Keep the wipe on a timer (`RespawnTimer`), not immediate: the hex arena's fall-death teleports the corpse to `FallRespawn` right after `Death()`, and the delay lets that land before `RespawnGroup` moves everyone to the checkpoint.
- `RespawnGroup()` — the wipe timer callback: teleports the group to `TargetPoint.Entrance` for `CheckpointTag` (**always**, via `GeoLib::TeleportPlayersToTargetPoints` with no exempt zone — there is no exempting someone from a respawn), revives everyone, and `RequestWaitingToStart()`. It never dereferences an arena.
- `CheckpointTag` / `SetCheckpointTag()` — the `Arena.*` tag a wipe respawns at. The arena owning the current fight registers it in its `StartFight()` (`SetCheckpointTag(ArenaTag)`), so the GameState needs no arena pointer to respawn — and an arena is free to register a *different* tag (advancing the checkpoint to the next room on victory). Read only during a respawn, so it never replicates.
- `RevivePlayers()` — iterates all player controllers and calls `Revive()` on each pawn. Called from `OnRep_MatchState` (leaving a fight) and from `RespawnGroup`; `Revive()` no-ops on a living player, so overlapping calls are free.
- `RequestWaitingToStart()` — the one way to stand a match down, used by the arena on victory (`AGeoArena::OnBossDefeated`) and by `RespawnGroup()` on a wipe. Both roads out of a fight lead to `WaitingToStart`. **Never `WaitingPostMatch`**: `AGameMode::StartMatch()` early-outs on `HasMatchStarted()`, which is **true in `WaitingPostMatch`**, so parking there makes every subsequent `TriggerAggro` a silent no-op and no further fight can begin.
- `CommitFightTime` — seconds from arena close to fight commit; read by `AGeoArena::StartFight` as its commit countdown and by `AGeoArenaBarrier::Tick` as the closing lerp duration. It stays here because it is a **plain timing constant two different actors read**, not an arena reference. `DeathTime` (the wipe delay, and the barrier's opening lerp duration) lives next to it.
- `OnMatchStateChanged` (`FMatchStateChanged`, two params: `MatchState`, `PreviousMatchState`) — broadcast on every match state transition, server and clients alike. This is the seam the arena (`EndFight`) and `UGeoCombatStatsSubsystem` subscribe to. Subscribe here instead of overriding `OnRep_MatchState` on clients.
