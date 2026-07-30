# GameClasses

Core Unreal game framework classes.

## Files
| File | Role |
|---|---|
| `GeoGameMode.h` | Game rules, player class assignment; `Tick` override |
| `GeoGameState.h` | Replicated match lifecycle + player death policy; holds no arena pointer |
| `GeoGameInstance.h` | Persistent across levels; `LeaveSessionAndReturnToMenu()` destroys Steam session then opens `MainMenuMap` |
| `GeoPlayerController.h` | Local-player setup (cursor, input mode, view target, gameplay mapping context, key seeding) runs in **`ReceivedPlayer`, never `BeginPlay`** — a couch-coop player joining mid-game spawns their controller into a world that has already begun play, so `BeginPlay` fires *before* `SetPlayer` and `GetLocalPlayer()` is still null there. Owns pause menu widget; toggled via `ToggleMenuAction`; swaps gameplay `InputMapping` for `MenuInputMapping` while open so abilities can't fire behind the menu. `ToggleMenuAction` is bound **only for local player 0** (one shared view = one pause menu); cursor + `SeedKeyBindingsForKeyboardLayout` gated on `GeoLib::IsKeyboardMousePlayer` |
| `GeoGameViewportClient.h` | Couch-coop device assignment + force-disabled splitscreen |
| `GeoPlayerState.h` | **Hosts ASC + attribute sets for playable characters** |
| `GeoMainMenuGameMode.h` | Menu level GameMode; no pawn |
| `GeoMainMenuPlayerController.h` | Menu controller; creates `UGeoMainMenuWidget` on `BeginPlay` |

## `GeoGameViewportClient` — couch coop
Local coop is **not a parallel code path**: a second `ULocalPlayer` gives a second PlayerController, PlayerState (with its own ASC), HUD and pawn, so GAS, replication, the arena and the death policy are untouched. This class only decides *which device owns which player*.

- `Config/Windows/WindowsInput.ini` sets `input.DeviceMappingPolicy=2` (`CreateUniquePlatformUserForEachDevice`). The Windows engine default is `1` (`PrimaryUserSharesKeyboardAndFirstGamepad`), which welds the first gamepad to player 1 — that makes "player 1 on keyboard, player 2 on the only gamepad" impossible. The policy is read **once into a static** (`GetDeviceMappingPolicyFromConfig`), so it must be config, never runtime.
- The engine routes input strictly by platform user and **never falls back to player 0** (`UEngine::GetLocalPlayerFromInputDevice` returns null), so a gamepad owning no local player is simply dropped. `InputKey` runs before `Super` and intercepts exactly that press.
- **`ApplyCouchCoopSetting()` is the whole rule**, and it reads `UGeoGameUserSettings::UseFirstGamepadForSecondPlayer` (default off):
  - **Off** — every gamepad is remapped onto the keyboard's platform user, reproducing the pre-coop `PrimaryUserSharesKeyboardAndFirstGamepad` behaviour on top of policy 2: pad and mouse both drive player 1, and Start joins nobody because the pad's user already owns one. Any existing player 2 is removed, since their pad just went back to player 1.
  - **On** — the *lowest-id* gamepad keeps `SecondPlayerUser` (allocated once via `AllocateNewUserId`, reused across toggles so flipping cannot exhaust the 8 platform users) and joins as player 2 on **Start** (`EKeys::Gamepad_Special_Right`). Every **other** gamepad still collapses onto the keyboard's user — that is what makes "2 players, 2 pads" work: pad A is player 2, pad B drives player 1 alongside the idle keyboard.
- It is called from `Init`, from `UGeoKeyBindingsWidget` on toggle, and **from `InputKey` whenever a gamepad key arrives with no owning local player**. That last call is not a fallback: a pad plugged in after launch gets a fresh platform user from the mapper, and re-deriving its owner there — before `Super::InputKey` routes it — is what stops its first press being dropped.
- `APlayerController::GetPlatformUserId()` reads through to its LocalPlayer, so controllers follow their player for free. **All platform-user juggling lives here** — `IPlatformInputDeviceMapper` needs an explicit `ApplicationCore` dependency (Engine's is conditional and does not propagate), which only this module takes, so the UI module must go through this class rather than calling the mapper itself.
- `bFilterInputByPlatformUser=True` (`DefaultInput.ini`, engine default is False) is what makes a "removed" keyboard actually dead and stops one player's sticks leaking into another.
- `SetForceDisableSplitscreen(true)` — every local player shares one view, framed by `AGeoGameCamera`. Leave `bOffsetPlayerGamepadIds` at its default false or `RemapControllerInput` starts rewriting device ids.
- **Slate focus is the other half of routing, and is easy to miss.** Each platform user gets its own `FSlateUser` with its own focus; a pad whose Slate user has no focused widget never reaches the viewport at all — its input becomes Slate menu navigation and drives the editor/UI instead of the game. `UGameViewportClient::NotifyPlayerAdded` only focuses Slate user `PlayerIndex`, which stops being the right user under policy 2, and never fires for a pad that has no local player yet (the join press). `ReceivedFocus` is overridden to call `SetAllUserFocusToGameViewport()`: it also arms `FSlateApplication::LastAllUsersFocusWidget`, which `RegisterNewUser` copies onto every Slate user created afterwards — so a pad's *first* press already lands on the viewport. It has to be `ReceivedFocus` and not `NotifyPlayerAdded`: Slate's game viewport widget is registered (`PlayLevel.cpp` / `UGameEngine::SwitchGameWindowToUseGameViewport`) *after* the initial local player is added, so at `NotifyPlayerAdded` time `SetAllUserFocusToGameViewport` silently no-ops on an invalid widget. Standalone also gets one call from `UGameEngine`; PIE only does it when Editor Preferences → Play → *Game Gets Mouse Control* is on, which is why the override is needed.
- **PIE**: Editor Preferences → Play → *Route gamepad to 2nd window* forwards all gamepad input to the next PIE viewport, so joining can never fire. Turn it off, or test with `DebugCreatePlayer 1`.

## `GeoPlayerState` — Critical for GAS
`UGeoAbilitySystemComponent`/`UCharacterAttributeSet` live here, not on the character (standard GAS multiplayer pattern — ASC survives respawn).
- `PlayerClass` (replicated) — `OnRep_PlayerClass` triggers class-switch visuals.
- `TeamId` — canonical team; `GetGenericTeamId()` reads it directly so attitude queries resolve even when the pawn is momentarily absent.
- Combat stats (replicated): smoothed DPS/HPS, `MaxBurstDamage`/`MaxBurstHealing` (biggest 0.5s-window total, i.e. biggest single spell), fight-average DPS/HPS, totals.

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
