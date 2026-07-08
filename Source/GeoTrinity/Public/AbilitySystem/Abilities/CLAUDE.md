# Abilities

All gameplay ability classes. **Base class for everything: `UGeoGameplayAbility`** (`Base/GeoGameplayAbility.h`). Never use `UGameplayAbility` directly — it bypasses StoredPayload, fire flow, CommitAbility, and cost infrastructure.

## Subfolders
| Folder | Contents |
|---|---|
| `Base/` | `GeoGameplayAbility`, `PatternAbility`, `AbilityPayload` — see `Base/CLAUDE.md` |
| `Damaging/` | `GeoProjectileAbility`, `GeoAutomaticFireAbility`, `GeoAutomaticProjectileAbility` |
| `Boss/` | `GeoPeriodicFireAbility`, `GeoDevastatingWaveAbility`, `GeoSpawnPillarAbility` — boss abilities |
| `Pattern/` | `Pattern`, `SpiralPattern`, `SpawnPillarPattern`, `DevastatingWavePattern` — deterministic enemy bullet patterns |
| `Circle/` | `GeoHealingAuraAbility`, `GeoMoiraBeamAbility`, `GeoChargeBeamAbility`, `GeoHealReturnPassiveAbility` |
| `Square/` | `GeoShieldBurstPassiveAbility`, `GeoDetonateWallsAbility` (wall deploy uses shared `GeoDeployAbility`) |
| `Triangle/` | `GeoReloadAbility`, `GeoRecallTurretAbility` |
| `Common/` | `GeoDashAbility`, `GeoDeployAbility` — shared across all classes |

## Fire Flow (all abilities)
```
ActivateAbility(TriggerEventData)
  ← Origin, Yaw, ServerSpawnTime, Seed already set from TriggerEventData
  ← server already has this data — no RPC needed yet
  CommitAbility()                  ← cost + cooldown, once at activation
  ScheduleFireTrigger()            ← waits FireDelay (or ChargeForFireDelay mode)
    Fire()                         ← client: spawns predicted projectile
      [optional] SendFireDataToServer()
        OnFireTargetDataReceived() ← server-only: spawns authoritative projectile
                                     with updated values (e.g. current aim yaw at fire time)
```

`SendFireDataToServer` / `OnFireTargetDataReceived` are **not mandatory** — only used when an ability needs a value that changes during `FireDelay` (e.g. projectile aim direction). The server already has the activation-time data from `TriggerEventData`.

## Passive abilities
Passives (`Ability.Type.Passive` tag) are server-owned: activated by `ReactivatePassiveAbilities()` (GiveLife) and set `NetSecurityPolicy = ServerOnly` in their C++ constructor. Without it, the client-side `CancelAllAbilities()` in Death/ReviveLogic sends `ServerCancelAbility` that kills the server's freshly reactivated instance after a revive (client processes the revive after the server already reactivated).

## Effect Data on Abilities
- `TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets` — shared/reused effects (asset references)
- `TArray<TInstancedStruct<FEffectData>> EffectDataInstances` — ability-specific inline effects
- `GetEffectDataArray()` merges both into one array for `ApplyEffectFromEffectData()`
