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
World subsystem tracking DPS / HPS / damage-received per player with **fixed-size state — no per-event storage**. Current DPS/HPS are exponentially smoothed rates (`SmoothingWindowSeconds` = 3 s time constant: each event adds `Amount / T`, rates decay by `e^(-dt/T)`), plus whole-combat averages (`FightDPS`/`FightHPS` = totals / time since `CombatStartTime`):
- `OnWorldBeginPlay` — server-only: subscribes to `GeoGameState::OnMatchStateChanged`
- Match `InProgress` starts a fight: `ResetStats()` zeroes everything and restarts `CombatStartTime`. Leaving `InProgress` ends it: `StatsPerActor.Empty()` **without** pushing zeros, so the player states keep displaying the final fight values
- `ReportDamageDealt` / `ReportHealingDealt` / `ReportDamageReceived` — decay the rates to now, fold the event in, push stats. The first event outside a match opens a **new session** (`FindOrAddStats` → `ResetStats`), zeroing the previous fight's frozen values for every player
- `ComputePlayerStats(CurrentTime)` — `DecayRates` + `PushPlayerStats` (best rates + fight averages + `SetDebugCombatStats`). Ticked every frame from `AGeoPlayerController::Tick` (non-shipping, `Geo.ShowCombatStats`); **no-op while `StatsPerActor` is empty** (right after a fight ends), which is what freezes the final fight values
- Reports come from `UGeoAttributeSetBase::PostGameplayEffectExecute()` on every damage/heal event

### `GeoSessionSubsystem.h`
**GameInstance** subsystem (survives level travel) — the direct-IP **no-Steam** connect/host seam. Steam sessions stay in `UGeoGameInstance` (CreateAdvancedSession/JoinAdvancedSession); this subsystem never touches them.
- `HostListen(MapPackagePath)` — **starts a listen server**: `ServerTravel`s the local player to `<MapPackagePath>?listen`, so the host is the authority and also plays. No separate process is launched; no Server target needed for local play. Joiners connect over direct IP via `JoinByAddress`.
- `JoinByAddress(Address)` — `ClientTravel` the local player to a host IP (engine default port 7777 if omitted). Used by Join-by-IP.
- `GetLocalIPv4()` — best-effort local IPv4 (via `ISocketSubsystem::GetLocalHostAddr`) for the host to read out; `"127.0.0.1"` on failure
- Called by `UGeoLocalConnectWidget` (the "Play Local" menu panel). Requires the `"Sockets"` module dep.
- Net driver: `DefaultEngine.ini` keeps SteamSockets as primary but falls back to `IpNetDriver`, so no-Steam instances network over plain IP. **Host and joiner must be in the same mode** (both Steam or both no-Steam) — mixed connections fail.
