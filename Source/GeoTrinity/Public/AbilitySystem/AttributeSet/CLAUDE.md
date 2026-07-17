# AbilitySystem/AttributeSet

---

## `GeoAttributeSetBase.h` — shared attributes (all characters)

Attributes: `Health`, `MaxHealth`, `Shield`, `IncomingDamage` (meta), `IncomingHeal` (meta)

**`PreAttributeChange()`** clamps `Shield` to `[0, MaxHealth]` before any modification lands, so shield can never exceed max life regardless of source (GE modifier, setter, OnRep).

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
| `AppliedHealBoost` | Captured by `ExecCalc_Heal` as source multiplier (healer's outgoing boost) |
| `ReceivedHealBoost` | Captured by `ExecCalc_Heal` as target multiplier (recipient's incoming boost) |
| `DamageMultiplier` | Captured by `ExecCalc_Damage` as source multiplier |
| `DamageReduction` | Captured by `ExecCalc_Damage` as target reduction |
| `MovementSpeedMultiplier` | Read by `UGeoCharacterMovementComponent::ApplySpeedMultiplier()` |
| `RotationSpeedMultiplier` | Read by `APlayableCharacter::UpdateAimRotation()` |
| `SacrificeValue` | Damage captured by the Square's sacrifice channel, consumed by the sacrifice detonation. Replicated for HUD display; zeroed on death (`DeathLogic`) |
| `HealCharge` | Healing recorded since the Circle's `GeoSweetSpotChargePassiveAbility` gauge was last consumed; a full gauge grants the charge beam's next sweet-spot release the passive's damage-multiplier boost. Replicated for HUD gauge display |
