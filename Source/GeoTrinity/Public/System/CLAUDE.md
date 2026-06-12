# System

World subsystems and pooling interface.

## Files

### `GeoActorPoolingSubsystem.h`
World subsystem for actor reuse:
- `RequestActor<T>(Class, Transform, Owner, Instigator)` — get from pool or spawn new
- `ReleaseActor(Actor)` — return to pool
- `PreSpawn<T>(Class, Count)` — pre-allocate
- `Get(World)` / `Get(WorldContextObject)` — singleton access

Internally: `TMap<UClass*, TArray<TWeakObjectPtr<AActor>>> Pool`.

### `GeoPoolableInterface.h`
Interface actors must implement to be poolable:
- `Init()` — called on acquisition
- `End()` — called on return

### `GeoCombatStatsSubsystem.h`
World subsystem tracking DPS / HPS / damage-received per player using a **10-second rolling window**:
- `OnWorldBeginPlay` — server-only: subscribes to `GeoGameState::OnMatchStateChanged` to reset all stats when a fight starts (`MatchState::InProgress`)
- `ReportDamageDealt(PlayerState, Amount)` — adds a timestamped event to the rolling window
- `ReportDamageReceived(PlayerState, Amount)` — increments the session total only (no rolling window for received damage)
- `ReportHealingDealt(PlayerState, Amount)` — adds a timestamped event to the rolling window
- `ComputePlayerStats(CurrentTime)` — prune old events, update session-best rolling DPS/HPS, push stats to `AGeoPlayerState`
- Called from `UGeoAttributeSetBase::PostGameplayEffectExecute()` on every damage/heal event
- `FActorCombatStats` holds `TArray<FCombatEventRecord>` for DamageDealt and HealingDealt; TotalDamageReceived is a plain running total (not a rolling window)

### `GeoSessionSubsystem.h`
**GameInstance** subsystem (survives level travel) — the direct-IP **no-Steam** connect/host seam. Steam sessions stay in `UGeoGameInstance` (CreateAdvancedSession/JoinAdvancedSession); this subsystem never touches them.
- `HostDedicated(MapPackagePath)` — **launches a dedicated server behind, then joins it as a pure client** (the local player is never the authority, so authority-gated HUD init runs like a remote joiner's). In-editor it `CreateProc`s `UnrealEditor-Cmd.exe "<uproject>" <MapPackagePath> -server -log -port=7777 -nosteam`; the `FProcHandle` is tracked and killed in `Deinitialize` so quitting the client never orphans a server. Packaged `GeoTrinityServer.exe` launch is gated behind `ensureMsgf(GIsEditor)` as a TODO.
- `JoinByAddress(Address)` — `ClientTravel` the local player to a host IP (engine default port 7777 if omitted). Used by Join-by-IP and internally by `HostDedicated` (loopback `127.0.0.1:7777`).
- `GetLocalIPv4()` — best-effort local IPv4 (via `ISocketSubsystem::GetLocalHostAddr`) for the host to read out; `"127.0.0.1"` on failure
- Called by `UGeoLocalConnectWidget` (the "Play Local" menu panel). Requires the `"Sockets"` module dep.
- Net driver: `DefaultEngine.ini` keeps SteamSockets as primary but falls back to `IpNetDriver`, so no-Steam instances network over plain IP. **Host and joiner must be in the same mode** (both Steam or both no-Steam) — mixed connections fail.
