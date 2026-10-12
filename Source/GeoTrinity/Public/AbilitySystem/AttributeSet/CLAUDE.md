# AbilitySystem/AttributeSet

| File | Role |
|---|---|
| `GeoAttributeSetBase.h` | All characters: `Health`, `MaxHealth`, `Shield`, meta `IncomingDamage` / `IncomingHeal` |
| `CharacterAttributeSet.h` | Players only (on `AGeoPlayerState` beside the ASC) |
| `GeoGemAttributeSet.h` | Stats only gems change; each starts at its no-gem value (multipliers at 1) |

## Base
- `PreAttributeChange` clamps `Shield` to `[0, MaxHealth]`.
- `PostGameplayEffectExecute` on `IncomingDamage`, in this order: `AGeoGameState::OnIncomingDamage` (stage 1: a listener takes the hit over, the Martyr redirect; the hit ends when all of it is taken) → `OnIncomingDamageShare` (stage 2: a listener takes a part, Bond) → shield absorption → clamp health → report to `UGeoCombatStatsSubsystem`, end actor at 0.
- Both stages are one signature: `(VictimASC, Context, Damage reaching the stage, float& Spared)`; a listener adds the part it takes onto itself to `Spared`; the hub clamps `Spared` to `Damage`.
- Two stages, not one: listeners of one multicast have no order, and Bond must see only what the redirect left (a sacrificed victim's hit is whole the Square's).
- The attribute set names no ability: a passive that reacts to a hit binds a stage.
- Meta attributes: transient, written by ExecCalc, zeroed after `PostGameplayEffectExecute`, read nowhere else.

## Character attributes
| Attribute | Purpose |
|---|---|
| `Ammo` / `MaxAmmo` | Triangle basic attack cost |
| `AppliedHealBoost` / `ReceivedHealBoost` | `ExecCalc_Heal` source / target multipliers |
| `DamageMultiplier` / `DamageReduction` | `ExecCalc_Damage` source / target |
| `MovementSpeedMultiplier` | `UGeoCharacterMovementComponent::ApplySpeedMultiplier()` |
| `SacrificeValue` | Captured sacrifice damage; replicated, zeroed on death |
| `HealCharge` | Circle sweet-spot gauge; replicated |

## Gems
- A new gem stat = one attribute here + its catalog row; `UGeoGemStatsEffect` lists every attribute of this set.

## Death
- Attribute delegates fire synchronously: a lethal `SetHealth` runs `OnHealthChanged` → match teardown before the rest of `PostGameplayEffectExecute` (direct-Health GEs even before it is called). End a match / fight from a death callback with `SetTimerForNextTick` (`AGeoArena::OnBossDefeated`); code after `SetHealth` may run after teardown.
