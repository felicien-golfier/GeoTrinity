# AbilitySystem/Types

## `GeoAscTypes.h` — `FGeoGameplayEffectContext`

Extends `FGameplayEffectContext` with GeoTrinity-specific data.

### Standard hit info
- `bIsBlockedHit`, `bIsCriticalHit`
- `StatusTag` — gameplay tag of applied status
- `Icon` — replicated texture shown in the HUD status bar while the applied effect is active; set per-spec by `FGameplayEffectData::ApplyEffect` (via a duplicated context) and by `ApplyStatusToTarget` (from `FRpgStatusInfo.Icon`); read client-side by `AGeoHUD::GetActiveEffectIcons`
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
| `bIsFromBasicAbility` | `FDamageEffectData::UpdateContextHandle` (when ability's owned tags include `Ability.Basic`) | `ExecCalc_Damage` → records hit target on source ASC via `SetLastBasicAbilityTarget`; used by `AGeoTurret::FindBestTarget` |
| `bDoNotRedirectSacrifice` | `FDamageEffectData` (redirect shares from `UGeoSacrificeBeamAbility`, deployable life drain, any damage that must not be captured) | `UGeoSacrificeBeamAbility::TryRedirectIncomingDamage` (called from `PostGameplayEffectExecute`) — skips sacrifice capture |

### `GeoAbilitySystemGlobals` (`Globals/GeoAbilitySystemGlobals.h`)
- `AllocGameplayEffectContext()` — allocates `FGeoGameplayEffectContext` instead of the default engine context. This is what makes all GEs use the custom context automatically.
- `InitGameplayCueParameters()` — populates GC params with Instigator, EffectCauser, hit location/normal.
