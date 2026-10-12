# Abilities

Root of every ability. Parent class always `UGeoGameplayAbility` (`Base/`), never `UGameplayAbility`.

| Folder | Holds |
|---|---|
| `Base/` | `GeoGameplayAbility`, `GeoChannelBeamAbility`, `PatternAbility`, `AbilityPayload` |
| `Damaging/` | Projectile / auto-fire bases |
| `Common/` | Shared by all classes: dash, deploy, triggered effect, deploy cues |
| `Circle/` `Square/` `Triangle/` | Per-class kits (Healer / Tank / DPS) |
| `Gem/` | Core-gem passives |
| `Boss/` | Server-only boss abilities |
| `Pattern/` | Enemy bullet/hazard pattern objects (`Projectile/`, `Hazard/`) |

## Fire flow
```
ActivateAbility(TriggerEventData)   payload set; server has it, no RPC
  CommitAbility()                   cost + cooldown, once
  ScheduleFireTrigger()             FireDelay or charge release
    Fire()                          client: predicted spawn
      [SendFireDataToServer()]      only when a value changes during FireDelay
        OnFireTargetDataReceived()  server: authoritative spawn
```

## Passives
- Tag `Ability.Type.Passive`; server-owned, `NetSecurityPolicy = ServerOnly`.
- Activated by `ReactivatePassiveAbilities()` on `GiveLife()`.
- HUD frame lit only when the passive has something armed: `IsReady` (`Base/CLAUDE.md`, "HUD readiness"). No cooldown and no override → never lit.

## Effects on abilities
- `EffectDataAssets` = shared `UEffectDataAsset`s; `EffectDataInstances` = inline.
- `GetEffectDataArray()` merges both → `ApplyEffectFromEffectData()`.
