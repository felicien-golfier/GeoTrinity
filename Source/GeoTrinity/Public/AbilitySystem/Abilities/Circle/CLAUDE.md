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
- `SweetSpotDamageMultiplier` — separate multiplier applied instead when released within the sweet-spot window; when `GeoSweetSpotChargePassiveAbility`'s gauge is full, the base damage entries are replaced by the passive's `GetBoostDamage` (multiplier drops to 1) and the gauge is consumed at the end of `DealDamage`, hit or miss
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
- Binds to `OnHealProvided` on the owner's ASC (server). The first heal after consumption starts a `ChargeDuration`-second charge window (`HealChargeStartTime` attribute = server world time, 0 = idle); healing done inside the window accumulates into `HealCharge`; heals after the window elapses are ignored until consumption
- Gauge fill is **time-based**: `GetGaugeRatio(ASC)` = elapsed/`ChargeDuration` (server world time from the ASC's active-effects container, consistent on clients)
- Full gauge: the charge beam's next sweet-spot release deals `GetBoostDamage(ASC)` = `HealToDamageRatio` × `HealCharge` **instead of its base damage**, then `ConsumeGauge(ASC)` zeroes both attributes (called in `UGeoChargeBeamAbility::DealDamage`; also on `EndAbility`, so death/class change clears the gauge)
- `FindOnASC(ASC)` (static) — resolves the granted passive on an ASC
- HUD: `AGeoHUD::GetActiveEffectIcons` appends a synthetic status-bar entry (`GaugeIcon`, `FillRatio`, `GaugeFullColor`); the status bar reveals the icon bottom-to-top with the fill and tints it `GaugeFullColor` when full
