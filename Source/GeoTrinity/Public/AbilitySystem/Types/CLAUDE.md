# AbilitySystem/Types

## `GeoAscTypes.h` — `FGeoGameplayEffectContext`

Extends `FGameplayEffectContext` with GeoTrinity-specific data.

### Standard hit info
- `bIsBlockedHit`, `bIsCriticalHit`
- `StatusTag` — gameplay tag of applied status
- Debuff: `DebuffDamage`, `DebuffDuration`, `DebuffFrequency`
- Physics: `DeathImpulseVector`, `KnockbackVector`
- Radial: `bIsRadialDamage`, `InnerRadius`, `OuterRadius`, `Origin`

### Call-site scoped fields
These are **not replicated** and **not included in `NetSerialize`**. They are embedded into the spec via `Duplicate()` so they survive `MakeOutgoingSpec` and reach `ExecCalc`. They auto-reset per `ApplyEffectFromEffectData` call (fresh context each call).

| Field | Set by | Read by |
|---|---|---|
| `SingleUseDamageMultiplier` | `FSingleUseDamageMultiplierEffectData::UpdateContextHandle` | `ExecCalc_Damage` |
| `bSuppressHealProvided` | `FHealEffectData::UpdateContextHandle` | `ExecCalc_Heal` |
| `bSuppressGameplayCue` | `FDamageEffectData` / `FHealEffectData` | `ExecCalc_*` (unconditional suppress) |
| `bLimitGameplayCue` | `FDamageEffectData` / `FHealEffectData` | `ExecCalc_*` (rate-limits via target's `UGeoGameFeelComponent`) |
| `bSuppressCombatStats` | `FDamageEffectData` / `FHealEffectData` | `UGeoAttributeSetBase::PostGameplayEffectExecute` |

### `GeoAbilitySystemGlobals` (`Globals/GeoAbilitySystemGlobals.h`)
- `AllocGameplayEffectContext()` — allocates `FGeoGameplayEffectContext` instead of the default engine context. This is what makes all GEs use the custom context automatically.
- `InitGameplayCueParameters()` — populates GC params with Instigator, EffectCauser, hit location/normal.
