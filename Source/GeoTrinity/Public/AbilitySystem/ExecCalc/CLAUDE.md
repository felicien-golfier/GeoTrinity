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

---

## `ExecCalc_Heal.h`

Captures:
- `HealMultiplier` from **source** `UCharacterAttributeSet`

Applies to `IncomingHeal` meta-attribute on `UGeoAttributeSetBase`.

Broadcasts `OnHealProvided(Amount)` on source ASC **unless** `bSuppressHealProvided` is set on the context (used by `GeoHealReturnPassiveAbility` to avoid infinite heal loops).
