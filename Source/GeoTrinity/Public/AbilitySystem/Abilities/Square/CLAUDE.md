# Abilities/Square

Square (Tank) class abilities.

---

## `GeoMineAbility.h` — proximity mine deployer
Extends `GeoDeployAbility`.
- Costs **50% of current HP** to deploy (not a flat cost — percentage of current health)
- `CheckCost()` override — prevents activation when at minimum health
- Mine spawned via `ADeployableSpawnerProjectile` → `AGeoMine`

---

## `GeoShieldBurstPassiveAbility.h` — passive burst shield
Auto-attack damage dealt fills a gauge; at 100% a shield burst is sent to nearby allies.

Key fields:
- `GaugeFillThreshold` — damage needed to fill gauge
- `ShieldAmount` (`FScalableFloat`) — shield granted per burst; scales with ability level
- `EnemyBounceMultiplier` — multiplier applied to `AGeoShieldBurstProjectile` per enemy bounce
- `ChargeTime = 1s` — wind-up before burst fires

Uses `UShieldBurstPassiveComponent` for the replicated `GaugeRatio` visual on the character material.

Flow:
1. Ability binds to `OnDamageDealt` delegate on owner's ASC
2. Damage increments internal gauge
3. At 100%: `Charge()` starts, `ChargeTime` later → spawns `AGeoShieldBurstProjectile` toward nearest ally
4. Projectile bounces off enemies (multiplying `ShieldAmount`), shields first ally it hits

---

## `GeoDetonateAllMinesAbility.h` — detonate all mines instantly
- Calls `Recall(true, Value)` on every `AGeoMine` in `UGeoDeployableManagerComponent`
- `DetonationMultiplier` (default 2x) — passed as `Value` to `Recall()`; scales damage/shield on explosion
