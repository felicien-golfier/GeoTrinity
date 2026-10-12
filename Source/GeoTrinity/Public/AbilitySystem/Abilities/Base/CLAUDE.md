# Abilities/Base

| File | Role |
|---|---|
| `GeoGameplayAbility.h` | Base of all abilities: payload, fire flow, fire modes, commit behaviour, effect data |
| `GeoChannelBeamAbility.h` | Channelled beam base (Moira beam, sacrifice beam): beam VFX lifecycle, windup preview, line scan |
| `PatternAbility.h` | Server-driven enemy pattern launcher; multicasts pattern start |
| `AbilityPayload.h` | `FAbilityPayload` per-shot snapshot, `FPatternData` polymorphic pattern data |

## GeoGameplayAbility
- Two data stages: `ActivateAbility` (TriggerEventData → payload) then optional `SendFireDataToServer` after `FireDelay`.
- `StoredPayload` is the only source of Owner/Origin/Yaw/Seed/Level/Instigator; copies of Owner named `PayloadOwner`.
- RNG: `FRandomStream(StoredPayload.Seed)` so client and server agree.
- `FireMode`: `ShootAfterFireDelay` | `ChargeForFireDelay` (release fires; `GetChargeRatio()` 0..1 via `GaugeChargingSpeedCurve`).
- `SetChargeGaugeVisible` virtual — override to bind another gauge widget.
- Montage sections cycle Start → Fire1 → Fire2…; counter lives on ASC per ability tag; play rate fits `FireDelay`.
- `CommitBehaviour`: `AtActivate` | `CostAtActivateCooldownAtEnd` | `DoNotAutoCommit`.
- `bActivateOnFreshPressOnly` for abilities sharing one input.
- Server-driven abilities (`ServerInitiated`/`ServerOnly`/passive) activate without TriggerEventData and build payload from avatar state (`STTask_FireAbility` path).
- `OnFireTargetDataReceived` is server-only by design; `Fire()` always sends the RPC.
- `GetFireOrigin2D` / `GetFireYaw` are the override points for aim.

## GeoChannelBeamAbility
- Server creates/destroys the replicated `UGeoBeamVFXComponent` in `OnGiveAbility` / `OnRemoveAbility`.
- Windup preview shows the shared Ray Zone Indicator sized to `GetFireDelay()`.
- Active beam ticks `TickBeam(DeltaTime, ActorsInLine)` (pure virtual); hooks `GetCurrentBeamHalfWidth`, `GetScanAttitudeMask`.
- Does not call `UGeoGameplayAbility::Fire`; subclasses needing a later RPC call `SendFireDataToServer` themselves.

## PatternAbility
- Enemy only; `PatternToLaunch` class instantiated on every machine via `PatternStartMulticast(Payload, Class, CreatePatternData())`.
- `CreatePatternData()` computes pattern-specific data once on server; pattern reads it via `StoredPatternData.GetPtr<T>()`.
- Fill derived data with explicit template: `TInstancedStruct<FPatternData>::Make<FDerived>(...)`.
- `EndAbility` force-stops the pattern before `Super`.
