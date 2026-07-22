# Abilities/Base

Foundation for the entire ability system.

---

## `GeoGameplayAbility.h` — base for ALL abilities

### Fire flow — two data stages
**Stage 1 — `ActivateAbility` (TriggerEventData):** payload fields (`Origin`, `Yaw`, `ServerSpawnTime`, `Seed`) set once from `TriggerEventData`. Server already has this — no RPC needed yet.
**Stage 2 — `SendFireDataToServer` after `FireDelay` (optional):** only when a value genuinely changes during the delay, e.g. current aim yaw at fire time. Many abilities need no second sync.

```
ActivateAbility → CommitAbility() → ScheduleFireTrigger() → [FireDelay] → Fire() (client, predicted spawn)
  → [optional] SendFireDataToServer() → ServerSetReplicatedTargetData RPC → OnFireTargetDataReceived() (server-only, authoritative spawn)
```

### FireMode
- `ShootAfterFireDelay` — fixed delay
- `ChargeForFireDelay` — fires on release; `GetChargeRatio()` 0..1 eased via `GameDataSettings::GaugeChargingSpeedCurve`. On release, jumps montage to end section (`GeoASLib::SectionEndName`) for the locally controlled player only, to avoid holding the last charge frame.
- `SetChargeGaugeVisible(Character, bVisible)` — virtual, called in `ScheduleFireTrigger` (show)/`EndAbility` (hide) for `ChargeForFireDelay`. Base is client-only no-op on server. Override for a different widget (see `UGeoChargeBeamAbility`).

### StoredPayload (`FAbilityPayload`)
Set once per activation. **Always use `StoredPayload` fields** — never `GetAvatarActor()` inside abilities (payload is client-set and may differ from ActorInfo). Local copies of `Owner` must be named `PayloadOwner`. Fields: `Owner`, `Origin`, `Yaw`, `ServerSpawnTime`, `Seed`, `AbilityTag`, `AbilityLevel`, `Instigator`, `PredictionKey`.

### RNG
Always `FRandomStream Stream(StoredPayload.Seed)`. Never `FMath::Rand*` directly — client/server must derive identical values from the same seed.

### Animation
Section cycling Start → Fire1 → Fire2 → … loops; montages with no Fire section reset to 0 instead of incrementing (`GetFireSectionIndex` detects absence). Tracked on ASC per ability tag. Play rate auto-adjusted to fit `FireDelay`.

### Effect Data
`EffectDataAssets` (shared) + `EffectDataInstances` (inline) → `GetEffectDataArray()` merges both for `ApplyEffectFromEffectData()`.

### Key Invariants
- Activation without `TriggerEventData` is legal for server-driven abilities: `ActivateAbility` asserts target data exists unless passive or `NetExecutionPolicy` is `ServerInitiated`/`ServerOnly` (`bIsServerDriven`) — those build `StoredPayload` from avatar state. `STTask_FireAbility` activates by tag with no event data, so every AI-fired ability lands here. `UPatternAbility` sidesteps entirely (doesn't call `Super::ActivateAbility`).
- `OnFireTargetDataReceived` is server-only by design — never add `IsServer` guards inside it.
- Never override `Fire()` as a no-op — it sends the RPC that triggers `OnFireTargetDataReceived`.
- `bActivateOnFreshPressOnly` — Held input never activates; only a fresh press does. Used when abilities share one input (sacrifice channel/detonate) to prevent held-button chain-activation.
- `CommitBehaviour`: `AtActivate` (default, cost+cooldown on activate), `CostAtActivateCooldownAtEnd` (cooldown deferred to `EndAbility`), `DoNotAutoCommit` (subclass calls `CommitAbility()` manually).

---

## `GeoChannelBeamAbility.h` — base for channelled beam abilities
Shared infra for `GeoMoiraBeamAbility` (Circle) and `GeoSacrificeBeamAbility` (Square):
- `OnGiveAbility`/`OnRemoveAbility` — server creates/destroys the replicated `UGeoBeamVFXComponent` (`BeamNiagaraSystem`+`BeamColor` feed Niagara `User.Color`)
- `Fire()` sets `bIsBeamActive`; `EndAbility` clears it + `SetBeamState(false,…)`; `FTickableGameObject` ticks only while active
- `Tick()` pushes beam VFX state, calls pure-virtual `TickBeam(DeltaTime, ActorsInLine)` from `GeoASLib::GetInteractableActorsInLine`
- Virtual hooks: `GetCurrentBeamHalfWidth`, `GetScanAttitudeMask` (default `All`)
- `Fire()` does **not** call `UGeoGameplayAbility::Fire` — channel beams don't send fire target data on start; subclasses needing a later RPC (sacrifice detonation) call `SendFireDataToServer` themselves.

---

## `PatternAbility.h` — server-driven enemy bullet patterns
Enemy-only. `UPatternAbility` is the ability class; `UPattern`/`UTickablePattern` are pattern objects (`Pattern/`).
- `PatternToLaunch` — class to instantiate
- `ActivateAbility` → `CreateAbilityPayload()` → `PatternStartMulticast(Payload, PatternToLaunch, CreatePatternData())` RPC → all clients create a `UPattern`, call `InitPattern`; binds `OnPatternEnd`
- `EndAbility` override calls `PatternInstance->EndPattern(true)` before `Super` — force-stops without triggering a recursive end chain
- **Subclass hook**: override `GetFireOrigin2D`/`GetFireYaw` on `UGeoGameplayAbility` to change target data
- **Per-pattern data hook**: override `CreatePatternData()` for pattern-specific replicated data, computed once on the server so all clients agree. Base returns unset `TInstancedStruct<FPatternData>`. Each pattern defines its own `FPatternData` subclass, read via `StoredPatternData.GetPtr<T>()`. See `FPatternData` in `AbilityPayload.h`.

---

## `AbilityPayload.h` — per-shot networking snapshot
`FAbilityPayload` — carried by `StoredPayload` and `FGeoAbilityTargetData`. Fields: `Origin` (`FVector2D`), `Yaw`, `ServerSpawnTime`, `Seed`, `AbilityLevel`, `AbilityTag`, `Owner`, `Instigator`, `PredictionKey`.

`FPatternData` — empty polymorphic base for pattern-specific data sent alongside `FAbilityPayload` through `PatternStartMulticast`. Subclass with `UPROPERTY` fields, fill server-side in `CreatePatternData()`, read via `StoredPatternData.GetPtr<T>()`.

**When filling a derived `FPatternData`, always spell the type out: `TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(Derived)`.** Omitting the template arg resolves to the variadic overload defaulting `T` to base `FPatternData`, slicing the derived struct — `GetPtr<Derived>()` then returns null everywhere (looks like "InstancedStruct arrives empty"; the slicing happens at creation, not over the wire).
