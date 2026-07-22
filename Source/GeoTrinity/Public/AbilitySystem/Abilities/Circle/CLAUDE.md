# Abilities/Circle

Circle (Healer) class abilities.

---

## `GeoHealingAuraAbility.h` — passive healing aura
`FTickableGameObject`, ticks independently. Heals allies overlapping the character's capsule each tick. `HealPerSecond` scales with ability level via curve. Activated at class-load, never deactivated while alive.

## `GeoMoiraBeamAbility.h` — fire-and-forget beam
Extends `UGeoChannelBeamAbility` (owns beam VFX lifecycle, tick boilerplate, line scan); implements `TickBeam` (zone drain, damage/heal) and overrides `GetCurrentBeamHalfWidth` (capsule/2 + growth). Damages enemies/heals allies in a forward rectangle; can absorb `AGeoHealingZone` actors to grow. Detection via `GetInteractableActorsInLine`; length = `GameDataSettings::GeneralSpellDistance`.

Key fields: `DamagePerSecond`/`HealPerSecond` (level-scaled), `SpeedBuffEffect` (self infinite GE on activation), `InitialDuration`, `DurationPerAbsorbedZone`, `HalfWidthGrowthPerAbsorbedZone`, `DamageAndHealBoostPerAbsorbedZone`, `BeamZoneDrainPercentagePerSecond`, `MaximumZoneAbsorbed`, `BeamNiagaraSystem`.

`UGeoBeamVFXComponent` — `NewObject` on server in `OnGiveAbility`, destroyed in `OnRemoveAbility`, never cached (fetched via `FindComponentByClass`). `Tick` calls `SetBeamState` with current half-width (server write replicates); `EndAbility` calls `SetBeamState(false,…)`.

Runtime state: `BeamRatio` (grows with absorbed zones), `RemainingDuration`.

## `GeoChargeBeamAbility.h` — chargeable single-shot beam
Hold to charge, release to fire (`ChargeForFireDelay`). **Hit detection is server-side** — `OnFireTargetDataReceived` iterates hostile actors in front and applies effects directly; no projectile spawned.

- `SweetSpotMinRatio`/`MaxRatio` (0.5/0.7 default) — window granting `MaxDamageMultiplier`
- `MinDamageMultiplier`/`MaxDamageMultiplier` — lerped by charge ratio for non-sweet-spot releases
- `SweetSpotDamageMultiplier` — applied instead within the window; when `GeoSweetSpotChargePassiveAbility`'s gauge is full, its boost (lerped by `GetSweetSpotPrecision()`) adds on top and the gauge is consumed at end of `DealDamage`, hit or miss
- Charge ratio encoded in `StoredPayload.Seed` as integer permillage 0..1000, set at release in `GetUpdatedTargetData`; `GetStoredChargeRatio()`/`IsSweetSpotRelease()` decode it back
- `FireGameplayCue` fires beam VFX/SFX on the locally-controlled client only
- Overrides `SetChargeGaugeVisible` to bind `ChargeBeamGaugeComponent` instead of the deploy gauge

## `GeoHealReturnPassiveAbility.h` — passive self-heal return
Binds `OnHealProvided` on owner's ASC; on each heal, returns `SelfHealPercent` (0..1) back to self. Always active while Circle.

## `GeoSweetSpotChargePassiveAbility.h` — passive sweet-spot boost gauge
Binds `OnHealProvided` (server); each heal accumulates `HealCharge`, capped at `HealRequiredForFullCharge`. `GetGaugeRatio(ASC)` = `HealCharge/HealRequiredForFullCharge`.

Full gauge: next sweet-spot charge-beam release adds `GetHealsToDamageMultiplier(Precision)` = lerp(`EdgeDamageMultiplierBoost`, `CenterDamageMultiplierBoost`, precision) **on top of** `SweetSpotDamageMultiplier`; `ConsumeGauge(ASC)` then zeroes `HealCharge` (called in `DealDamage`, and on `EndAbility` so death/class-change clears it). Resolve via `GeoASLib::GetGrantedAbility<UGeoSweetSpotChargePassiveAbility>(ASC)`.

HUD: `AGeoHUD::GetActiveEffectIcons` appends a synthetic status-bar entry (`GaugeIcon`, `FillRatio`, `GaugeFullColor`) — status bar reveals it bottom-to-top and shines over-bright when full.
