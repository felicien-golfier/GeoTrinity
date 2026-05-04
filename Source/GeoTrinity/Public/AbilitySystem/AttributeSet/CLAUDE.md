# AbilitySystem/AttributeSet

---

## `GeoAttributeSetBase.h` — shared attributes (all characters)

Attributes: `Health`, `MaxHealth`, `Shield`, `IncomingDamage` (meta), `IncomingHeal` (meta)

**`PostGameplayEffectExecute()` does three things:**
1. Clamps `Health` to `[0, MaxHealth]`
2. Reports damage/heal to `UGeoCombatStatsSubsystem`
3. Ends the actor (calls `Destroy` or equivalent) when health reaches 0

`GetHealthRatio()` — returns `Health / MaxHealth`, 0 when `MaxHealth = 0`.

Meta-attributes (`IncomingDamage`, `IncomingHeal`) are transient — they accumulate the ExecCalc result and are reset to 0 after `PostGameplayEffectExecute`. Never read them outside of `PostGameplayEffectExecute`.

---

## `CharacterAttributeSet.h` — player-only attributes

Lives on `AGeoPlayerState` alongside the ASC.

| Attribute | Purpose |
|---|---|
| `Ammo` / `MaxAmmo` | Triangle's ammo system; basic attack costs 1 ammo per shot |
| `HealMultiplier` | Captured by `ExecCalc_Heal` as source multiplier |
| `DamageMultiplier` | Captured by `ExecCalc_Damage` as source multiplier |
| `DamageReduction` | Captured by `ExecCalc_Damage` as target reduction |
| `MovementSpeedMultiplier` | Read by `UGeoCharacterMovementComponent::ApplySpeedMultiplier()` |
| `RotationSpeedMultiplier` | Read by `APlayableCharacter::UpdateAimRotation()` |
