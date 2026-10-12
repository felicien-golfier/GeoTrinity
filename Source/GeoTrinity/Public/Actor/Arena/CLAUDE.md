# Actor/Arena

One boss encounter and everything room-shaped. The arena drives itself; GameState only owns match state and death policy (`GameClasses/Match/CLAUDE.md`).

| File | Role |
|---|---|
| `GeoArena.h` | Base arena: owns boss + adds, barrier, fight commit, boss bar, loot, floor look |
| `GeoHexArena.h` | Arena on a destructible hex tile grid |
| `GeoDummyArena.h` | Training dummy: spawns its enemy, `IsBoss() = false`, no fight machinery |
| `GeoArenaBarrier.h` | Replicated barrier lerping transforms Fight-On ↔ Fight-Off |
| `GeoHexBarrier.h` | Barrier variant: hex-tile alley vanishing tile by tile |
| `GeoArenaVolume.h` | Box naming which arena players stand in (checkpoint/respawn tag outside a fight) |

## GeoArena
- Reached by abilities via `GetArenaOfBoss(Boss)` (reads `AEnemyCharacter::Arena`). One boss per arena.
- Fight driven by replicated `bFighting`:
  - `StartFight()` (from `AGeoEnemyAIController::TriggerAggro`): sets `bFighting`, boss bar, GameState checkpoint tag, closes barrier, arms `CommitFightTimer`; refuses when already fighting.
  - `CommitFight()`: teleports players to `TargetPoint.FightLocation` (skips those inside `FightZoneTagName`).
  - `EndFight()` (from `AGeoGameState::OnMatchStateChanged`, server): clears timer, `bFighting` false, hides bar; resets the boss only on a wipe (victory sets `bBossDefeated`).
  - `RespawnBoss()` / static `RespawnAllBosses()`: only way back after a victory (`GeoMatchStateButton` `RespawnBosses`, or new server).
- Loot: `OnBossDefeated` starts a looping shower until any fight starts (`StopLoot()`); `GetFightingArena()` = static "who is fighting".
- `IsBoss()` is virtual and the sole gate on match start (checked in `TriggerAggro`).
- Finished attempts go to `UGeoLeaderboardSave`.

## GeoHexArena
- Tiles are NoCollision visuals over a flat invisible floor; replicated `TileStates` (`FHexTileState`: `bAlive`, `bHighlighted`) is truth.
- `ApplyTileVisuals()` diffs and zero-scales dead tiles (never remove instances). Server calls it manually after mutating.
- Server tick = one rule: anything unsupported over a hole falls (players die → respawn at `TargetPoint.FallRespawn`, except mid-dash; deployables `Recall()` / `Expire()`).
  - `IsSupported`: within `FallGraceMargin` of the shared border of the alive neighbour (found O(1) by cube coordinates; distance perpendicular to the border).
  - Tick enabled by `CommitFight`, disabled + reset by `EndFight`.
- Telegraph: `HighlightTiles(Requester, Center, Radius)` / `ClearHighlight`, server-only bookkeeping keyed by requester (`HighlightRequests`); only the union bool replicates.
- `TileOccupants` (server-only) keeps `GetRandomAliveTiles` from stacking deployables; freed via `OnDeployableExpiredEvent`.

## Barriers
- `SetClosed(bool)` server-only; lerp driven by the actor's own `Tick`, enabled from `OnRep_bIsClosed` (called manually on authority). Times from GameState `CommitFightTime` / `DeathTime`.
- `OnBarrierStateChanged` is VFX only.
- `GeoHexBarrier`: rides inherited `bIsClosed`; alley must lie inside the arena's `FallCheckRadius` if players should fall through. Shape = `TileLayout` ((Column, Row) in vanish order); sculpt by deleting instances then `CaptureLayout` button, before anything re-runs the construction script.
