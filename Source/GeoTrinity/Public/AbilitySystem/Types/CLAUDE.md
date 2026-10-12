# AbilitySystem/Types

`GeoAscTypes.h` — `FGeoGameplayEffectContext : FGameplayEffectContext`. Allocated for every GE by `UGeoAbilitySystemGlobals` (`Globals/`, registered in `DefaultGame.ini`; it also fills cue params with instigator, causer, hit location/normal).

## Replicated
- `StatusTag` (`GeoASLib::Get/SetStatusTag`).
- `Icon` — status-bar texture; set by `FGameplayEffectData::ApplyEffect`, read by `AGeoHUD::GetActiveEffectIcons`.

## Call-site scoped (not replicated)
Copied by `Duplicate()` into the outgoing spec; reset each `ApplyEffectFromEffectData`.

| Field | Set by | Read by |
|---|---|---|
| `SingleUseDamageMultiplier` | `FSingleUseDamageMultiplierEffectData` | `ExecCalc_Damage` |
| `bSuppressHealProvided` | `FHealEffectData` | `ExecCalc_Heal` |
| `bSuppressGameplayCue` | damage / heal data | `ExecCalc_*` |
| `bLimitGameplayCue` | damage / heal data | `ExecCalc_*` (via `UGeoGameFeelComponent`) |
| `bSuppressCombatStats` | damage / heal data | `UGeoAttributeSetBase::PostGameplayEffectExecute` |
| `bIsFromBasicAbility` | `FDamageEffectData` (ability owns `Ability.Basic`) | `ExecCalc_Damage` → `SetLastBasicAbilityTarget` |
| `bDoNotRedirectSacrifice` | redirect shares, deployable life drain | sacrifice redirect |
