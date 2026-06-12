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
