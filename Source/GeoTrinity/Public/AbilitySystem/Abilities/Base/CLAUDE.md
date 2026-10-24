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
- Montage ownership: `ShouldPlayAbilityMontage()` (virtual, default true) gates `HandleAnimationMontage`. True = the ability plays `AnimMontage` through `ASC->PlayMontage` on every machine that runs it, server included, so it replicates to remote copies. A pattern ability answers false when its pattern has a montage: the pattern plays it locally per machine (`Pattern/CLAUDE.md`), so a replicated one would restart it at the server's section and rate.
- An ability with no pattern (bomb, turret, mine, player kits) keeps the replicated montage. It must not cut a montage already playing on the same avatar: author it in its own slot group, since `Montage_Play` stops only its own group's montages (`MCP_Animation.md`, "slot group").
- `CommitBehaviour`: `AtActivate` | `CostAtActivateCooldownAtEnd` | `DoNotAutoCommit`.
- `bActivateOnFreshPressOnly` for abilities sharing one input.
- Server-driven abilities (`ServerInitiated`/`ServerOnly`/passive) activate without TriggerEventData and build payload from avatar state (`STTask_FireAbility` path).
- `OnFireTargetDataReceived` is server-only by design; `Fire()` always sends the RPC.
- `GetFireOrigin2D` / `GetFireYaw` are the override points for aim.

### Owner death (`OnOwnerDeath`)
- `OnOwnerDeath()` virtual, called on every active instance by `UGeoAbilitySystemComponent::HandleOwnerDeath` when the owner dies; runs on every machine, like `DeathLogic`.
- Default: end cancelled (`CancelAbility`), so anything with a live effect (beam, channel, charge, montage) stops.
- Override to stay active (no-op) or to reset state and stay; the ability owns the rule, the death path never names a class.
- Staying active does not lift the `IsDead()` refusal in `CanActivateAbility`: a dead owner can never start one.
- Every passive that reads positions, contacts or the owner's health pool must answer for the dead owner itself (override, or `IsDead()` check in its handler).

### HUD readiness (`IsReady`, `GetClassGauge`, `GetGaugeLabel`)
- `IsReady(ASC) const`: true = slot frame lit. One rule per ability; the UI never names an ability class.
- `GetClassGauge(ASC, OutFill, bOutReady) const`: false = no class gauge (default). True on the one ability per class that feeds the player card's class ring and its own slot / passive tile gauge (same fill on both).
- `GetGaugeLabel(ASC, OutLabel) const`: the number written in the middle of that slot gauge and as the player card's gauge text; false = no number on either (default). A two-state gauge (fill 0 or 1) never writes its fill: it writes a count when ready, or nothing. Never "0 / X".
- Both called on the CDO on clients (passives are server-only instances): read only the given ASC and its avatar — attributes, GE tags and stacks, replicated components. Never instance members, never new replication.
- `IsReady` default: passive without `CooldownGameplayEffectClass` → false (nothing to show); otherwise → none of `GetCooldownTags()` on the ASC. Covers actives and cooldown passives.
- Overrides: deploy charges, sweet-spot, shield burst, sacrifice detonate (`Common/`, `Circle/`, `Square/` CLAUDE.md). Gauge and label overrides: sweet-spot, sacrifice detonate, recall turret.
- Gauge fill and readiness are separate answers: fill shows progress, `bOutReady` follows the ability's own ready rule (sacrifice: armed at any fill).

## GeoChannelBeamAbility
- Server creates/destroys the replicated `UGeoBeamVFXComponent` in `OnGiveAbility` / `OnRemoveAbility`.
- Windup preview shows the shared Ray Zone Indicator sized to `GetFireDelay()`.
- Active beam ticks `TickBeam(DeltaTime, ActorsInLine)` (pure virtual); hooks `GetCurrentBeamHalfWidth`, `GetScanAttitudeMask`.
- Does not call `UGeoGameplayAbility::Fire`; subclasses needing a later RPC call `SendFireDataToServer` themselves.

## PatternAbility
- Enemy only; `PatternToLaunch` class instantiated on every machine via `PatternStartMulticast(Payload, Class, CreatePatternData())`.
- `CreatePatternData()` computes pattern-specific data once on server; pattern reads it via `StoredPatternData.GetPtr<T>()`.
- Data given by the caller: an activation whose `TriggerEventData->OptionalObject` is an `IPatternDataSource` (`GeoActivateAbilityAction`) keeps its `GetPatternData()`; the base `CreatePatternData()` returns it (unset when none). One ability, its pattern configured per firing at runtime (lateral wave safe spots). An override computing its own data ignores it.
- Fill derived data with explicit template: `TInstancedStruct<FPatternData>::Make<FDerived>(...)`.
- `EndAbility` force-stops the pattern before `Super`.
- `ShouldPlayAbilityMontage()` is false when `PatternToLaunch`'s CDO `HasMontage()`; a pattern without montage leaves the ability's own `AnimMontage` replicated as for any ability.
