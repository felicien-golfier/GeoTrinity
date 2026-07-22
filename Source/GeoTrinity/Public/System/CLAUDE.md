# System

World subsystems and pooling interface.

### `GeoActorPoolingSubsystem.h`
World subsystem for actor reuse: `RequestActor<T>`, `ReleaseActor`, `PreSpawn<T>`, `Get(World)` singleton access. Internally `TMap<UClass*, TArray<TWeakObjectPtr<AActor>>> Pool`.

### `GeoPoolableInterface.h`
Interface for poolable actors: `Init()` on acquisition, `End()` on return.

### `GeoCombatStatsSubsystem.h`
World subsystem tracking DPS/HPS/damage-received per player with **fixed-size state, no per-event storage**. Current rates are exponentially smoothed (`SmoothingWindowSeconds=3s` time constant); plus whole-combat averages (`FightDPS`/`FightHPS` = totals / time since `CombatStartTime`).
- `OnWorldBeginPlay` (server-only): subscribes to `GeoGameState::OnMatchStateChanged`. Entering `InProgress` calls `ResetStats()`; leaving it does `StatsPerActor.Empty()` **without** pushing zeros, so player states keep displaying final fight values.
- `Report*` methods decay rates to now, fold event in, push stats; the first event outside a match opens a **new session** (zeroing previous fight's frozen values).
- `ComputePlayerStats` ticked every frame from `AGeoPlayerController::Tick` (non-shipping, `Geo.ShowCombatStats`); no-ops while `StatsPerActor` is empty (right after a fight ends) — that's what freezes final values.
- Reports come from `UGeoAttributeSetBase::PostGameplayEffectExecute()`.

### `GeoSessionSubsystem.h`
**GameInstance** subsystem (survives level travel) — direct-IP **no-Steam** connect/host seam; Steam sessions stay in `UGeoGameInstance` and are never touched here.
- `HostListen(MapPackagePath)` — `ServerTravel`s to `<Map>?listen`; host is authority and plays, no separate process.
- `JoinByAddress(Address)` — `ClientTravel` to host IP (default port 7777).
- `GetLocalIPv4()` — best-effort local IP for host to display; `"127.0.0.1"` on failure.
- Net driver: `DefaultEngine.ini` falls back to `IpNetDriver` when not using SteamSockets. **Host and joiner must be in the same mode** (both Steam or both no-Steam) — mixed connections fail.
