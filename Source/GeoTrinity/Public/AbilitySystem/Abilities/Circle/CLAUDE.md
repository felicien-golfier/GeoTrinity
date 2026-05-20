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
Damages enemies and heals allies inside a forward rectangle. Can absorb deployed `AGeoHealingZone` actors to grow.
Target detection uses `GeoASLib::GetInteractableActorsInLine`; beam length = `GameDataSettings::GeneralSpellDistance`.

Key fields:
- `DamagePerSecond` / `HealPerSecond` — per-second amounts; scale with ability level
- `SpeedBuffEffect` — infinite GE applied to self on activation (additive `MovementSpeedMultiplier`)
- `InitialDuration` — base channel duration (seconds)
- `DurationPerAbsorbedZone` — duration bonus per fully absorbed healing zone
- `HalfWidthGrowthPerAbsorbedZone` — half-width added per fully absorbed zone (cm)
- `DamageAndHealBoostPerAbsorbedZone` — `1.0 = 100%` damage/heal boost per zone
- `BeamZoneDrainPercentagePerSecond` — rate at which the beam drains zone health (0..100%/s)

Runtime state: `BeamRatio` (grows as zones are absorbed), `RemainingDuration`.

---

## `GeoChargeBeamAbility.h` — chargeable single-shot beam
Hold to charge, release to fire. Uses `ChargeForFireDelay` FireMode.
**Hit detection is server-side** — `OnFireTargetDataReceived` iterates hostile actors in front of the character and applies effects directly; no projectile is spawned.

- `SweetSpotMinRatio` / `SweetSpotMaxRatio` (0..1) — charge window that grants `MaxDamageMultiplier` on release
- `MinDamageMultiplier` / `MaxDamageMultiplier` — damage multiplier lerped by charge ratio; sweet-spot always gets max
- Charge ratio encoded in `StoredPayload.Seed` as integer 0..100 (set at release time in `GetUpdatedTargetData`)
- `GetChargeRatio()` on base ability returns 0..1 with easing curve applied
- `FireGameplayCue` — fires beam VFX/SFX on the locally-controlled client only (not from `OnFireTargetDataReceived`)

---

## `GeoHealReturnPassiveAbility.h` — passive self-heal return
- Binds to `OnHealProvided` delegate on owner's ASC
- On each heal event, returns `SelfHealPercent` (0..1) of the heal amount back to self
- Passive — always active while Circle class is active
