# Abilities/Circle

Circle (Healer) class abilities.

---

## `GeoHealingAuraAbility.h` — passive healing aura
Implements `FTickableGameObject` — ticks independently from the game tick.
- Heals allies physically overlapping the character's capsule each tick
- `HealPerSecond` scales with ability level via curve
- Passive — activated at class-load, never deactivated while alive

---

## `GeoMoiraBeamAbility.h` — fire-and-forget beam
Damages enemies and heals allies inside a forward cylinder. Can absorb deployed `AGeoHealingZone` actors to grow.

Key fields:
- `BeamLength` — cylinder depth (cm)
- `InitialDuration` — base duration (seconds)
- `DurationPerAbsorbedZone` — duration bonus per absorbed healing zone
- `RadialGrowthPerAbsorbedZone` — beam radius increase per absorption
- `DamageAndHealBoostPerAbsorbedZone` — `1.0 = 100%` damage/heal boost per zone
- `BeamZoneDrainPercentagePerSecond` — how fast the beam drains absorbed zone health (0..100%)

`IsInBeam(Actor)` — checks if actor center is inside the current beam cylinder.

---

## `GeoChargeBeamAbility.h` — chargeable single-shot beam
Hold to charge, release to fire. Uses `ChargeForFireDelay` FireMode.

- `SweetSpotMinRatio` / `SweetSpotMaxRatio` (0..1) — charge window that triggers bonus effect on release
- `MinDamageMultiplier` / `MaxDamageMultiplier` — lerped by charge ratio
- Charge ratio encoded in `StoredPayload.Seed` as integer 0..100 (set at release time)
- `GetChargeRatio()` on base ability returns 0..1 with easing curve applied

---

## `GeoHealReturnPassiveAbility.h` — passive self-heal return
- Binds to `OnHealProvided` delegate on owner's ASC
- On each heal event, returns `SelfHealPercent` (0..1) of the heal amount back to self
- Passive — always active while Circle class is active
