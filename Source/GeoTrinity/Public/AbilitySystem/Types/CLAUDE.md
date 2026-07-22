# AbilitySystem/Types

## `GeoAscTypes.h` — `FGeoGameplayEffectContext`
Extends `FGameplayEffectContext` with GeoTrinity data.

### Replicated hit info
- `StatusTag` — applied status tag, replicated via `GeoASLib::GetStatusTag`/`SetStatusTag`
- `Icon` — replicated texture shown in HUD status bar while effect active; set by `FGameplayEffectData::ApplyEffect` and `ApplyStatusToTarget`; read by `AGeoHUD::GetActiveEffectIcons`

### Call-site scoped fields
Not replicated, not in `NetSerialize`. Embedded via `Duplicate()` so they survive `MakeOutgoingSpec` and reach ExecCalc. Auto-reset each `ApplyEffectFromEffectData` call.

| Field | Set by | Read by |
|---|---|---|
| `SingleUseDamageMultiplier` | `FSingleUseDamageMultiplierEffectData` | `ExecCalc_Damage` |
| `bSuppressHealProvided` | `FHealEffectData` | `ExecCalc_Heal` |
| `bSuppressGameplayCue` | `FDamageEffectData`/`FHealEffectData` | `ExecCalc_*` (unconditional suppress) |
| `bLimitGameplayCue` | `FDamageEffectData`/`FHealEffectData` | `ExecCalc_*` (rate-limit via `UGeoGameFeelComponent`) |
| `bSuppressCombatStats` | `FDamageEffectData`/`FHealEffectData` | `UGeoAttributeSetBase::PostGameplayEffectExecute` |
| `bIsFromBasicAbility` | `FDamageEffectData` (ability owns `Ability.Basic`) | `ExecCalc_Damage` → `SetLastBasicAbilityTarget`, used by `AGeoTurret::FindBestTarget` |
| `bDoNotRedirectSacrifice` | `FDamageEffectData` (redirect shares, deployable life drain) | `UGeoSacrificeBeamAbility::TryRedirectIncomingDamage` — skips sacrifice capture |

### `GeoAbilitySystemGlobals` (`Globals/GeoAbilitySystemGlobals.h`)
- `AllocGameplayEffectContext()` — allocates `FGeoGameplayEffectContext` (makes all GEs use it automatically)
- `InitGameplayCueParameters()` — populates GC params with Instigator, EffectCauser, hit location/normal
