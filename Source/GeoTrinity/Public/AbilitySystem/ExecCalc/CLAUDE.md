# AbilitySystem/ExecCalc

Execution calculations — the math layer between GE application and attribute modification.

---

## `ExecCalc_Damage.h`

Captures:
- `DamageMultiplier` from **source** `UCharacterAttributeSet`
- `DamageReduction` from **target** `UCharacterAttributeSet`
- `SingleUseDamageMultiplier` from `FGeoGameplayEffectContext` (set by `FSingleUseDamageMultiplierEffectData::UpdateContextHandle`)

Applies to `IncomingDamage` meta-attribute on `UGeoAttributeSetBase`.

Final damage = `BaseDamage × DamageMultiplier × SingleUseDamageMultiplier × (1 - DamageReduction)`.

Also broadcasts `OnDamageDealt` on source ASC.

GameplayCue gating: suppresses the cue when `bSuppressGameplayCue` is set on the context, or when `bLimitGameplayCue` is set and the target's `UGeoGameFeelComponent::IsDamageCueAvailable()` reports the rate window is not elapsed.

---

## `ExecCalc_Heal.h`

Captures:
- `AppliedHealBoost` from **source** `UCharacterAttributeSet` (healer's outgoing heal boost)
- `ReceivedHealBoost` from **target** `UCharacterAttributeSet` (recipient's incoming heal boost)

Applies to `IncomingHeal` meta-attribute on `UGeoAttributeSetBase`.

Broadcasts `OnHealProvided(Amount)` on source ASC **unless** `bSuppressHealProvided` is set on the context (used by `GeoHealReturnPassiveAbility` to avoid infinite heal loops).

GameplayCue gating: same as `ExecCalc_Damage` — `bSuppressGameplayCue` suppresses unconditionally, `bLimitGameplayCue` rate-limits via the target's `UGeoGameFeelComponent::IsHealCueAvailable()`.
