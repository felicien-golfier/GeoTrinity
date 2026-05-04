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
- `ReportDamageDealt(PlayerState, Amount)`
- `ReportDamageReceived(PlayerState, Amount)`
- `ReportHealingDealt(PlayerState, Amount)`
- `ComputePlayerStats(CurrentTime)` — prune old events, push stats to `AGeoPlayerState`
- Called from `UGeoAttributeSetBase::PostGameplayEffectExecute()` on every damage/heal event
- Stats stored as `FActorCombatStats` with `TArray<FCombatEventRecord>` per event type
