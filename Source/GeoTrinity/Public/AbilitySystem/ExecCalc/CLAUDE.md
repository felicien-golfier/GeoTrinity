# AbilitySystem/ExecCalc

Math between GE application and attribute change; result goes to the `Incoming*` meta attributes.

| File | Formula / role |
|---|---|
| `ExecCalc_Damage.h` | `Base × DamageMultiplier × SingleUseDamageMultiplier × (1 - DamageReduction)` |
| `ExecCalc_Heal.h` | `Base × AppliedHealBoost × ReceivedHealBoost` |

- Damage: source captures `DamageMultiplier`, target `DamageReduction`, `SingleUseDamageMultiplier` from `FGeoGameplayEffectContext`.
- Heal: capture the boost from **Target** when the source is a non-character (deployable, pickup).
- Both broadcast `OnDamageDealt` / `OnHealProvided` on the source ASC (heal skipped on `bSuppressHealProvided`).
- Cue: suppressed by `bSuppressGameplayCue`, rate-limited by `bLimitGameplayCue` via `UGeoGameFeelComponent::IsDamageCueAvailable()` / `IsHealCueAvailable()`.
- `bIsFromBasicAbility` → `SetLastBasicAbilityTarget` on the source ASC.
