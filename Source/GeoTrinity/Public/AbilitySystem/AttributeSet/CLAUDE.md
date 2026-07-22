# AbilitySystem/AttributeSet

## `GeoAttributeSetBase.h` — shared attributes (all characters)
Attributes: `Health`, `MaxHealth`, `Shield`, `IncomingDamage` (meta), `IncomingHeal` (meta)

- `PreAttributeChange()` clamps `Shield` to `[0, MaxHealth]` before any modification lands.
- `PostGameplayEffectExecute()`: clamps `Health` to `[0, MaxHealth]`, reports damage/heal to `UGeoCombatStatsSubsystem`, ends the actor at 0 health.
- `GetHealthRatio()` — `Health/MaxHealth`, 0 when `MaxHealth = 0`.
- Meta-attributes (`IncomingDamage`, `IncomingHeal`) are transient — accumulate ExecCalc result, reset to 0 after `PostGameplayEffectExecute`. Never read outside it.

## `CharacterAttributeSet.h` — player-only attributes
Lives on `AGeoPlayerState` alongside the ASC.

| Attribute | Purpose |
|---|---|
| `Ammo` / `MaxAmmo` | Triangle's ammo; basic attack costs 1/shot |
| `AppliedHealBoost` | `ExecCalc_Heal` source multiplier (healer's outgoing boost) |
| `ReceivedHealBoost` | `ExecCalc_Heal` target multiplier (recipient's incoming boost) |
| `DamageMultiplier` | `ExecCalc_Damage` source multiplier |
| `DamageReduction` | `ExecCalc_Damage` target reduction |
| `MovementSpeedMultiplier` | Read by `UGeoCharacterMovementComponent::ApplySpeedMultiplier()` |
| `RotationSpeedMultiplier` | Read by `APlayableCharacter::UpdateAimRotation()` |
| `SacrificeValue` | Damage captured by Square's sacrifice channel, consumed on detonation. Replicated for HUD; zeroed on death |
| `HealCharge` | Heal accrued for Circle's `GeoSweetSpotChargePassiveAbility` gauge; full gauge boosts next sweet-spot release. Replicated for HUD gauge |
