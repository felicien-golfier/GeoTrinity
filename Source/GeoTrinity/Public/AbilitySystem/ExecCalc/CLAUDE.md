# AbilitySystem/ExecCalc

Execution calculations — the math layer between GE application and attribute modification.

## `ExecCalc_Damage.h`
Captures `DamageMultiplier` (source), `DamageReduction` (target), `SingleUseDamageMultiplier` (from `FGeoGameplayEffectContext`, set by `FSingleUseDamageMultiplierEffectData`). Applies to `IncomingDamage` meta-attribute.

`Final = BaseDamage × DamageMultiplier × SingleUseDamageMultiplier × (1 - DamageReduction)`.

Broadcasts `OnDamageDealt` on source ASC. Cue suppressed when `bSuppressGameplayCue`, or rate-limited when `bLimitGameplayCue` and target's `UGeoGameFeelComponent::IsDamageCueAvailable()` says not elapsed.

## `ExecCalc_Heal.h`
Captures `AppliedHealBoost` (source), `ReceivedHealBoost` (target). Applies to `IncomingHeal` meta-attribute.

Broadcasts `OnHealProvided(Amount)` unless `bSuppressHealProvided` (used by `GeoHealReturnPassiveAbility` to avoid infinite heal loops). Cue gating same pattern as damage, via `IsHealCueAvailable()`.
