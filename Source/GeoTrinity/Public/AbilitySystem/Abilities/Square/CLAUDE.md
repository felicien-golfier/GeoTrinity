# Abilities/Square

Square (Tank) class abilities.

---

## Wall deployer — uses `GeoDeployAbility` directly (no Square-specific subclass)
The wall is deployed by the shared `UGeoDeployAbility` (`Common/`) configured in BP with `DeployableActorClass = BP_Wall`.
- Normal **flat GAS cost/cooldown** — no HP sacrifice
- Wall spawned via `ADeployableSpawnerProjectile` → `AGeoWall`

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

## `GeoDetonateWallsAbility.h` — boosting ray
A ray in front of the character (like `GeoChargeBeamAbility`), length `GeneralSpellDistance`, width `LineHalfWidth`.
- **Pass 1**: counts the player's own `AGeoWall`s on the ray and **recalls** them (consumed, no explosion). Multiplier = `WallBoostMultiplier ^ WallCount` (multiplicative, like Moira's zone absorption).
- **Pass 2**: along the same ray, instantly deals `BaseDamage * Multiplier` to enemies and grants `BaseShield * Multiplier` shield to allies (inline `FDamageEffectData`/`FShieldEffectData`, mirroring `GeoMoiraBeamAbility`).
- Target scan: `GeoASLib::GetInteractableActorsInLine(... TeamAttitudeMask::All ...)`; walls skipped in pass 2.
- Single `FireRay()` is called from `Fire` (host/client) and `OnFireTargetDataReceived` (server). It counts walls everywhere (client needs the count for the cue scale), but **recalls walls + applies damage/shield only on the server**, and fires the cue **only on the locally-controlled client**.
- Ray VFX: `FireGameplayCueTag` (local cue, like `GeoChargeBeamAbility::FireGameplayCue`) — `CueParams.Location` = ray endpoint, `Normal` = aim direction, `RawMagnitude` = consumed wall count (lets the Niagara beam scale with walls eaten).
- `WallBoostMultiplier`, `LineHalfWidth`, `BaseDamage`, `BaseShield` (`FScalableFloat`, level-scaled), `FireGameplayCueTag` configured in BP.
