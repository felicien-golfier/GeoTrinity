# Abilities

All gameplay ability classes. Base for everything: `UGeoGameplayAbility` (`Base/GeoGameplayAbility.h`). Never use `UGameplayAbility` directly — bypasses StoredPayload, fire flow, CommitAbility, and cost infrastructure.

## Subfolders
| Folder | Contents |
|---|---|
| `Base/` | `GeoGameplayAbility`, `PatternAbility`, `AbilityPayload` — see `Base/CLAUDE.md` |
| `Damaging/` | `GeoProjectileAbility`, `GeoAutomaticFireAbility`, `GeoAutomaticProjectileAbility` |
| `Boss/` | `GeoPeriodicFireAbility`, `GeoDevastatingWaveAbility`, `GeoSpawnPillarAbility`, `GeoSweepBeamAbility`, `GeoTileBombAbility`, `GeoSpawnOnTileAbility` — boss abilities |
| `Pattern/` | `Pattern`, `SpiralPattern`, `SpawnPillarPattern`, `DevastatingWavePattern`, `BeamPattern`, `ConeSprayPattern` — deterministic enemy bullet patterns |
| `Circle/` | `GeoHealingAuraAbility`, `GeoMoiraBeamAbility`, `GeoChargeBeamAbility`, `GeoHealReturnPassiveAbility`, `GeoSweetSpotChargePassiveAbility` |
| `Square/` | `GeoShieldBurstPassiveAbility`, `GeoDetonateWallsAbility` (wall deploy uses shared `GeoDeployAbility`) |
| `Triangle/` | `GeoReloadAbility`, `GeoRecallTurretAbility` |
| `Common/` | `GeoDashAbility`, `GeoDeployAbility` — shared across all classes |

## Fire Flow (all abilities)
```
ActivateAbility(TriggerEventData)  ← Origin/Yaw/ServerSpawnTime/Seed set from TriggerEventData; server already has this, no RPC yet
  CommitAbility()                  ← cost + cooldown, once at activation
  ScheduleFireTrigger()            ← waits FireDelay (or ChargeForFireDelay mode)
    Fire()                         ← client: spawns predicted projectile
      [optional] SendFireDataToServer()
        OnFireTargetDataReceived() ← server-only: spawns authoritative projectile, updated values (e.g. current aim yaw)
```
`SendFireDataToServer`/`OnFireTargetDataReceived` are optional — only needed when a value changes during `FireDelay` (e.g. aim direction).

## Passive abilities
Passives (`Ability.Type.Passive` tag) are server-owned: activated by `ReactivatePassiveAbilities()` (GiveLife), `NetSecurityPolicy = ServerOnly` in C++ constructor. Without it, a client-side `CancelAllAbilities()` on revive can kill the server's freshly reactivated instance.

## Effect Data on Abilities
- `TArray<TSoftObjectPtr<UEffectDataAsset>> EffectDataAssets` — shared/reused effects
- `TArray<TInstancedStruct<FEffectData>> EffectDataInstances` — ability-specific inline effects
- `GetEffectDataArray()` merges both for `ApplyEffectFromEffectData()`
