# Abilities/Base

Foundation for the entire ability system.

---

## `GeoGameplayAbility.h` — base for ALL abilities

### Fire Flow
```
ActivateAbility(TriggerEventData)
  ← payload fields (Origin, Yaw, ServerSpawnTime, Seed) already populated here from TriggerEventData
  ← server already has this data at this point — no RPC needed yet
  CommitAbility()                  ← cost + cooldown, once per activation
  ScheduleFireTrigger()
    └─ timer expires (FireDelay)
         Fire()                    ← CLIENT: spawns predicted projectile
           [optional] SendFireDataToServer()
             └─ ServerSetReplicatedTargetData (RPC)
                  OnFireTargetDataReceived() ← SERVER ONLY: spawns authoritative projectile
                                               with updated data (e.g. more accurate aim yaw)
```

### Data flow — two stages, not one

**Stage 1 — `ActivateAbility` (TriggerEventData):**
All payload fields (`Origin`, `Yaw`, `ServerSpawnTime`, `Seed`) are set **once** from `TriggerEventData` when the ability activates. The server already has this data. No RPC is needed for the server to know the ability fired.

**Stage 2 — `SendFireDataToServer` after `FireDelay` (optional, ability-specific):**
Some abilities send a second, updated snapshot after the `FireDelay` timer expires. This is only needed when the value genuinely changes during the delay — e.g. a projectile ability that needs the player's **current aim yaw at the moment of fire**, not the yaw at activation. Do NOT treat this as mandatory infrastructure. Many abilities need no second sync at all.

### FireMode
- `ShootAfterFireDelay` — fires after a fixed delay
- `ChargeForFireDelay` — fires on release; `GetChargeRatio()` returns 0..1 (eased by `GameDataSettings::GaugeChargingSpeedCurve` via `ApplyChargingCurve`). On release, `InputReleased` also jumps the montage to the end section (`GeoASLib::SectionEndName`) for the locally controlled player — prevents the charge animation from holding its last frame.
- `SetChargeGaugeVisible(Character, bVisible)` — virtual hook called in `ScheduleFireTrigger` (show) and `EndAbility` (hide) when in `ChargeForFireDelay` mode. Base implementation is **client-only** (no-op on server via `GeoLib::IsServer` guard) — gauge is a local-player concern. Override to use a different widget (see `UGeoChargeBeamAbility`).

### StoredPayload (`FAbilityPayload`)
Set once per activation. **Always use `StoredPayload` fields** — never call `GetAvatarActor()` or similar inside abilities. The payload is set by the client and may differ from what ActorInfo reports.
- `Owner` — local variable copies must be named `PayloadOwner`, never `AvatarActor`
- `Origin`, `Yaw`, `ServerSpawnTime`, `Seed`, `AbilityTag`, `AbilityLevel`, `Instigator`, `PredictionKey`

### RNG
Always: `FRandomStream Stream(StoredPayload.Seed)`. Never call `FMath::Rand*` directly — both client and server must derive identical values from the same seed.

### Animation
- Section cycling: Start → Fire1 → Fire2 → … (loops back). Montages without any Fire section (e.g. a pure Start/End montage) are valid — `GetFireSectionIndex` detects the absence and resets to 0 instead of incrementing, so the montage always starts from the beginning.
- `GetFireSectionIndex(AbilityTag)` — reference tracked on ASC per ability tag; `UGeoGameplayAbility::GetFireSectionIndex(ASC, AnimInstance)` fetches it, advances if Fire sections exist and montage is playing, resets to 0 otherwise
- Play rate auto-adjusted so each section fits the `FireDelay` window

### Effect Data
- `EffectDataAssets` (`TArray<TSoftObjectPtr<UEffectDataAsset>>`) — shared reusable effects
- `EffectDataInstances` (`TArray<TInstancedStruct<FEffectData>>`) — inline ability-specific effects
- `GetEffectDataArray()` — merges both; pass result to `ApplyEffectFromEffectData()`

### Key Invariants
- **Activation without `TriggerEventData` is legal for server-driven abilities.** `ActivateAbility` asserts that target data exists, *unless* the ability is passive or its `NetExecutionPolicy` is `ServerInitiated` **or `ServerOnly`** (`bIsServerDriven`) — those build `StoredPayload` from avatar state instead. `STTask_FireAbility` activates by tag via `TryActivateAbilitiesByTag` and passes no event data, so every AI-fired `UGeoGameplayAbility` lands on that path. `UPatternAbility` sidesteps it entirely by not calling `Super::ActivateAbility`.
- `OnFireTargetDataReceived` is **server-only by design** — never add `IsServer` guards inside it
- Never override `Fire()` as a no-op — it sends the RPC that triggers `OnFireTargetDataReceived`; a no-op silently breaks the server chain
- `bActivateOnFreshPressOnly` — when true, the per-frame Held input never activates the ability; only a fresh press does (`UGeoAbilitySystemComponent::AbilityInputTagPressed`). For abilities sharing one input (sacrifice channel/detonate) so a held button cannot chain-activate the counterpart.
- `CommitBehaviour` (`ECommitBehaviour`) — controls when cost/cooldown are committed:
  - `AtActivate` (default) — both cost and cooldown committed on `ActivateAbility`
  - `CostAtActivateCooldownAtEnd` — cost at activation, cooldown deferred to `EndAbility`
  - `DoNotAutoCommit` — subclass is responsible for calling `CommitAbility()` manually

---

## `GeoChannelBeamAbility.h` — base for channelled beam abilities

Shared infrastructure for `GeoMoiraBeamAbility` (Circle) and `GeoSacrificeBeamAbility` (Square):
- `OnGiveAbility`/`OnRemoveAbility` — server creates/destroys the replicated `UGeoBeamVFXComponent` on the avatar (`BeamNiagaraSystem` + `BeamColor` config; the color feeds the Niagara `User.Color` parameter)
- `Fire()` sets `bIsBeamActive`; `EndAbility` clears it and calls `SetBeamState(false, …)`; `FTickableGameObject` ticks only while active
- `Tick()` pushes the beam VFX state each tick, then calls the pure-virtual `TickBeam(DeltaTime, ActorsInLine)` with the result of `GeoASLib::GetInteractableActorsInLine`
- Virtual hooks: `GetCurrentBeamHalfWidth(Character)` (default: capsule radius / 2), `GetScanAttitudeMask()` (default: `All`)
- Like Moira originally, `Fire()` does **not** call `UGeoGameplayAbility::Fire` — channel beams don't send fire target data on start (the server starts its own channel from activation data); subclasses that need a later RPC (sacrifice detonation) call `SendFireDataToServer` themselves.

---

## `PatternAbility.h` — server-driven enemy bullet patterns

Enemy-only. `UPatternAbility` is the ability class; `UPattern`/`UTickablePattern` are the pattern objects (in `Pattern/` folder).

- `PatternToLaunch` — class to instantiate
- `ActivateAbility` → calls `CreateAbilityPayload()` → calls `PatternStartMulticast(Payload, PatternToLaunch, CreatePatternData())` RPC → all clients create a `UPattern` instance and call `InitPattern(Payload, PatternData)`; binds `OnPatternEnd` delegate
- `EndAbility` (override) → calls `PatternInstance->EndPattern(true)` before `Super` — force-stops the pattern (stops montages, skips `OnPatternEnd` broadcast) to avoid a recursive end chain when the ability itself is cancelled
- **Subclass hook**: override `GetFireOrigin2D(AActor*)` / `GetFireYaw(AActor const*)` on `UGeoGameplayAbility` to change what values the ASC bundles into target data on activation.
- **Per-pattern data hook**: override `CreatePatternData()` to ship pattern-specific replicated data through the multicast. Computed **once on the server**, so every client's `InitPattern` reads identical values instead of recomputing from locally-replicated state. Base returns an unset `TInstancedStruct<FPatternData>` (no extra data). Each pattern defines its own `FPatternData` subclass with `UPROPERTY` fields; the pattern reads it via `StoredPatternData.GetPtr<T>()`. Example: `UGeoSpawnPillarAbility` resolves zone locations here so all clients spawn zones at the same positions. See `FPatternData` in `AbilityPayload.h`.

---

## `AbilityPayload.h` — per-shot networking snapshot

`FAbilityPayload` — carried by both `StoredPayload` (on ability) and `FGeoAbilityTargetData` (RPC payload).

Fields: `Origin` (`FVector2D`), `Yaw`, `ServerSpawnTime`, `Seed`, `AbilityLevel`, `AbilityTag`, `Owner`, `Instigator`, `PredictionKey`

`FPatternData` (same header) — empty polymorphic base for **pattern-specific** data sent alongside `FAbilityPayload` through `PatternStartMulticast`. Patterns that need extra replicated data subclass it with `UPROPERTY` fields, fill it server-side in `UPatternAbility::CreatePatternData()`, and read it back via `StoredPatternData.GetPtr<T>()`. `FAbilityPayload` stays concrete (all ability code reads its fields directly); the polymorphism lives only on this sibling — mirrors `FDeployableDataParams` + `TInstancedStruct<FEffectData>`.

**When filling a derived `FPatternData` in `CreatePatternData()`, always spell the type out: `TInstancedStruct<FPatternData>::Make<FSpawnPillarPatternData>(Derived)`.** `Make(Derived)` without the explicit template arg resolves to the variadic `Make(TArgs&&...)` overload with `T` defaulted to the *base* `FPatternData`, which slices the derived struct down to the base — `GetPtr<Derived>()` then returns null on every machine (the bug looks like "InstancedStruct arrives empty"). The slicing happens at creation, not over the wire.
