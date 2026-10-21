# Actor/Arena

One boss encounter and everything room-shaped. The arena drives itself; GameState only owns match state and death policy (`GameClasses/Match/CLAUDE.md`).

| File | Role |
|---|---|
| `GeoArena.h` | Base arena: owns boss + adds, barrier, fight commit, boss bar, gem fountain, floor look |
| `GeoHexArena.h` | Arena on a destructible hex tile grid |
| `GeoDummyArena.h` | Training dummy: spawns its enemy, `IsBoss() = false`, no fight machinery; also hosts a room's `AHiddenCasterCharacter` (`Characters/CLAUDE.md`) |
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
- `OnBossDefeated` stores `LootOrigin` (the death position the fountain starts from); `GetFightingArena()` = static "who is fighting".
- Rewards: `BossLoot` (`FGeoBossLoot`, next to `BossType`) = this boss's random part (own drop table, or the catalog's for `BossType`) + fixed gems + fixed shards; `GrantGemRewards` hands every player the same kill loot. Rules: `Gem/CLAUDE.md` ("Boss loot"). `IsDataValid` (editor) flags fixed gems the catalog no longer has.
- Fountain (`Gem/CLAUDE.md`, "3D gems"): on a kill `GrantGemRewards` rolls the loot once (every player gets that same loot) and `StartFountain` queues one `AGeoGemDrop` per gem and per `ShardsPerDrop` shards (shuffled), thrown from `LootOrigin` (the boss's death position) by `SpawnFountainDrop`, a timer re-armed after each throw so the gaps follow `FountainFrontLoad` ("Fountain feel"): up at `FountainUpSpeed` ± `FountainUpSpread`, sideways in `FountainSideSpeed`, random direction; each drop carries a random `Seed` for its tumble. Class `GemDropClass` (default native; a Blueprint child sets gravity, spin, bob, pickup radius, scale).
- `IsBoss()` is virtual and the sole gate on match start (checked in `TriggerAggro`).
- Adds: `AddClasses` at reset, and `SpawnAdd(Class, Transform, Setup)` (server; `Setup` optional; spawns through `AEnemyCharacter::SpawnEnemy`, `Characters/CLAUDE.md` "Enemy setup") for one brought in mid-fight (a boss summon, a dead summon's ally). Every add is in `Adds`, `Arena` set: a wipe (`ResetBoss` → `ResetAdds`) and the boss's death (`DestroyAdds`) remove them all, so each attempt starts clean. `GetLiveAdds()` = the ones not dead (a corpse stays listed until destroyed); the list is server-side, a client gets none (`Pattern/Hazard/CLAUDE.md`, "LateralWave").
- Finished attempts go to `UGeoLeaderboardSave`, unless `bRecordsLeaderboard` is off (tutorial miniboss: not a real boss run).

## GeoHexArena
- Tiles are NoCollision visuals over a flat invisible floor; replicated `TileStates` (`FHexTileState`: `bAlive`, `bHighlighted`) is truth.
- `ApplyTileVisuals()` diffs and zero-scales dead tiles (never remove instances). Server calls it manually after mutating.
- The precise fall rule of this arena (dead tiles sit over an invisible floor, so nothing really falls through them); falling off real geometry is the world `KillZ` (`Characters/CLAUDE.md`, "Void").
- Server tick = one rule: anything unsupported over a hole falls (players `Death()`, except mid-dash or under root motion, then the GameState death policy respawns them; deployables `Expire()`). Runs only between `CommitFight` and `EndFight` (`bFallCheckEnabled`).
  - `IsSupported`: within `FallGraceMargin` of the shared border of the alive neighbour (found O(1) by cube coordinates; distance perpendicular to the border).
  - Tick enabled by `CommitFight`, disabled + reset by `EndFight`.
- Telegraph: `HighlightTiles(Requester, Center, Radius)` / `ClearHighlight`, server-only bookkeeping keyed by requester (`HighlightRequests`); only the union bool replicates.
- `TileOccupants` (server-only) keeps `GetRandomAliveTiles` from stacking deployables; freed via `OnDeployableExpiredEvent`.

## Barriers
- `SetClosed(bool)` server-only; lerp driven by the actor's own `Tick`, enabled from `OnRep_bIsClosed` (called manually on authority). Times from GameState `CommitFightTime` / `DeathTime`.
- `OnBarrierStateChanged` is VFX only.
- Every `AnimatedActors` actor (a door mesh) needs Movable mobility on its root component: a Static one never moves (PIE warning "has to be 'Movable' if you'd like to move") and the door silently stays at its fight-off place. Tutorial map doors (`Tut_*_Door`) are Movable; a script placing a door sets it.
- `GeoHexBarrier`: rides inherited `bIsClosed`; alley must lie inside the arena's `FallCheckRadius` if players should fall through. Shape = `TileLayout` ((Column, Row) in vanish order); sculpt by deleting instances then `CaptureLayout` button, before anything re-runs the construction script.
