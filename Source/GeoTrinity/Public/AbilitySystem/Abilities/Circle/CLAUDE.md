# Abilities/Circle

Circle (Healer) kit.

| File | Role |
|---|---|
| `GeoHealingAuraAbility.h` | Passive aura; own tick; heals allies overlapping the capsule, `HealPerSecond` by level curve |
| `GeoMoiraBeamAbility.h` | Channel beam (`GeoChannelBeamAbility`): damage hostiles / heal allies in a forward rectangle, absorbs `AGeoHealingZone`s to grow |
| `GeoChargeBeamAbility.h` | Hold-charge single-shot beam; hit detection server-side, no projectile |
| `GeoHealReturnPassiveAbility.h` | Returns `SelfHealPercent` of each heal to self |
| `GeoSweetSpotChargePassiveAbility.h` | Heal-fed gauge that boosts the next sweet-spot charge-beam release |

## Moira beam
- `TickBeam` does zone drain + damage/heal; `GetCurrentBeamHalfWidth` = capsule/2 + growth.
- Absorbed zones add duration, width and damage/heal boost.
- Beam length = `GameDataSettings::GeneralSpellDistance`.

## Charge beam
- `ChargeForFireDelay`; release ratio encoded in `StoredPayload.Seed` as permillage 0..1000 (`GetStoredChargeRatio`, `IsSweetSpotRelease`).
- Multiplier: lerp(`Min`,`Max`) by ratio; inside sweet-spot window `SweetSpotDamageMultiplier` instead.
- Full sweet-spot gauge adds boost lerped by `GetSweetSpotPrecision()`, consumed at end of `DealDamage`.
- Cue fires on the locally-controlled client only; overrides `SetChargeGaugeVisible` for its own gauge.

## Sweet-spot passive
- Resolve with `GeoASLib::GetGrantedAbility<UGeoSweetSpotChargePassiveAbility>(ASC)`.
- `ConsumeGauge` zeroes `HealCharge` (also on `EndAbility`).
- HUD shows it as a synthetic status entry via `AGeoHUD::GetActiveEffectIcons`.
- `IsReady` = `GetGaugeRatio >= 1`; `GetClassGauge` = Circle's class ring (fill = ratio, ready when full). Reads the replicated `HealCharge`.
- `GetGaugeLabel` = the ratio as a whole percentage ("42%"), shown on both the slot and the player card.
- `GeoHealReturnPassiveAbility`, `GeoHealingAuraAbility`: no cooldown, no override → passive tile never lit.
