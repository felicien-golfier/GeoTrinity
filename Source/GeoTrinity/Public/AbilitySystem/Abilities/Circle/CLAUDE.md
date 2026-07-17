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
Extends `UGeoChannelBeamAbility` (`Base/`) which owns the beam VFX component lifecycle, the tickable boilerplate, and the per-tick line scan; Moira implements `TickBeam` (zone drain, damage/heal) and overrides `GetCurrentBeamHalfWidth` (capsule/2 + growth) and `Tick` (fuel countdown).
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
- `MaximumZoneAbsorbed` — cap on how many HealingZone actors the beam may fully absorb in one activation; zone absorption stops once this count is reached
- `BeamNiagaraSystem` — Niagara system passed to the `UGeoBeamVFXComponent` in `OnGiveAbility`; assign a BP subclass of `GeoBeamVFXComponent` in the level if custom param names are needed
- `UGeoBeamVFXComponent` — created via `NewObject` on the server in `OnGiveAbility`, destroyed in `OnRemoveAbility`; never cached — fetched via `FindComponentByClass`. `Tick` calls `SetBeamState` with the current half-width (server write replicates to all clients); `EndAbility` calls `SetBeamState(false, …)` to switch it off

Runtime state: `BeamRatio` (grows as zones are absorbed), `RemainingDuration`.

---

## `GeoChargeBeamAbility.h` — chargeable single-shot beam
Hold to charge, release to fire. Uses `ChargeForFireDelay` FireMode.
**Hit detection is server-side** — `OnFireTargetDataReceived` iterates hostile actors in front of the character and applies effects directly; no projectile is spawned.

- `SweetSpotMinRatio` (default 0.5) / `SweetSpotMaxRatio` (default 0.7) — charge window that grants `MaxDamageMultiplier` on release
- `MinDamageMultiplier` / `MaxDamageMultiplier` — damage multiplier lerped by charge ratio for non-sweet-spot releases
- `SweetSpotDamageMultiplier` — separate multiplier applied instead when released within the sweet-spot window; when `GeoSweetSpotChargePassiveAbility`'s gauge is full, the passive's `GetHealsToDamageMultiplier` boost, lerped by `GetSweetSpotPrecision()` (1 at the window center, 0 at its edges), is added on top and the gauge is consumed at the end of `DealDamage`, hit or miss
- `GetStoredChargeRatio()` / `IsSweetSpotRelease()` — decode `StoredPayload.Seed` back to the 0–1 charge ratio / sweet-spot window test
- Charge ratio encoded in `StoredPayload.Seed` as integer permillage 0..1000 (set at release time in `GetUpdatedTargetData`)
- `GetChargeRatio()` on base ability returns 0..1 with easing curve applied
- `FireGameplayCue` — fires beam VFX/SFX (endpoint, charge ratio, sweet-spot flag) on the locally-controlled client only
- Overrides `SetChargeGaugeVisible` to bind `ChargeBeamGaugeComponent` instead of the deploy gauge

---

## `GeoHealReturnPassiveAbility.h` — passive self-heal return
- Binds to `OnHealProvided` delegate on owner's ASC
- On each heal event, returns `SelfHealPercent` (0..1) of the heal amount back to self
- Passive — always active while Circle class is active

---

## `GeoSweetSpotChargePassiveAbility.h` — passive sweet-spot boost gauge
- Binds to `OnHealProvided` on the owner's ASC (server). Every heal provided accumulates into the `HealCharge` attribute, capped at `HealRequiredForFullCharge`
- Gauge fill is **heal-based**: `GetGaugeRatio(ASC)` = `HealCharge`/`HealRequiredForFullCharge`, clamped to 1
- Full gauge: the charge beam's next sweet-spot release adds `GetHealsToDamageMultiplier(Precision)` = lerp(`EdgeDamageMultiplierBoost`, `CenterDamageMultiplierBoost`, precision) **on top of `SweetSpotDamageMultiplier`** — precision is how close the release lands to the sweet-spot center (`UGeoChargeBeamAbility::GetSweetSpotPrecision`) — then `ConsumeGauge(ASC)` zeroes `HealCharge` (called in `UGeoChargeBeamAbility::DealDamage`; also on `EndAbility`, so death/class change clears the gauge). The charge-beam gauge widget shades the sweet-spot window toward its center while the gauge is full
- Resolve the granted passive on an ASC with `GeoASLib::GetGrantedAbility<UGeoSweetSpotChargePassiveAbility>(ASC)`
- HUD: `AGeoHUD::GetActiveEffectIcons` appends a synthetic status-bar entry (`GaugeIcon`, `FillRatio`, `GaugeFullColor`); the status bar reveals the icon bottom-to-top with the fill in the icon's own colors and shines it over-bright `GaugeFullColor` when full
