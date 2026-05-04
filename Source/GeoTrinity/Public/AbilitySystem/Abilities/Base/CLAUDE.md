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
- `ChargeForFireDelay` — fires on release; `GetChargeRatio()` returns 0..1 (with easing from `GameDataSettings`)

### StoredPayload (`FAbilityPayload`)
Set once per activation. **Always use `StoredPayload` fields** — never call `GetAvatarActor()` or similar inside abilities. The payload is set by the client and may differ from what ActorInfo reports.
- `Owner` — local variable copies must be named `PayloadOwner`, never `AvatarActor`
- `Origin`, `Yaw`, `ServerSpawnTime`, `Seed`, `AbilityTag`, `AbilityLevel`, `Instigator`, `PredictionKey`

### RNG
Always: `FRandomStream Stream(StoredPayload.Seed)`. Never call `FMath::Rand*` directly — both client and server must derive identical values from the same seed.

### Animation
- Section cycling: Start → Fire1 → Fire2 → … (loops back)
- `GetFireSectionIndex(AbilityTag)` — reference tracked on ASC per ability tag
- Play rate auto-adjusted so each section fits the `FireDelay` window

### Effect Data
- `EffectDataAssets` (`TArray<TSoftObjectPtr<UEffectDataAsset>>`) — shared reusable effects
- `EffectDataInstances` (`TArray<TInstancedStruct<FEffectData>>`) — inline ability-specific effects
- `GetEffectDataArray()` — merges both; pass result to `ApplyEffectFromEffectData()`

### Key Invariants
- `OnFireTargetDataReceived` is **server-only by design** — never add `IsServer` guards inside it
- Never override `Fire()` as a no-op — it sends the RPC that triggers `OnFireTargetDataReceived`; a no-op silently breaks the server chain
- `CommitAbility()` is called **once** in `ActivateAbility()` — not per shot (except `UGeoAutomaticFireAbility`)

---

## `PatternAbility.h` — server-driven enemy bullet patterns

Enemy-only. `UPatternAbility` is the ability class; `UPattern`/`UTickablePattern` are the pattern objects (in `Pattern/` folder).

- `PatternToLaunch` — class to instantiate
- `ActivateAbility` → calls `PatternStartMulticast()` RPC → all clients create a `UPattern` instance
- Ability waits for `OnPatternEnd` delegate, then ends

---

## `AbilityPayload.h` — per-shot networking snapshot

`FAbilityPayload` — carried by both `StoredPayload` (on ability) and `FGeoAbilityTargetData` (RPC payload).

Fields: `Origin` (`FVector2D`), `Yaw`, `ServerSpawnTime`, `Seed`, `AbilityLevel`, `AbilityTag`, `Owner`, `Instigator`, `PredictionKey`
