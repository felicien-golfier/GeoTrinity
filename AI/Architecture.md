# Architecture Reference

Networking, data structures and the effect system. Class inventories and folder layout are in the root
`CLAUDE.md`.

## Networking

ASC lives on `AGeoPlayerState` for players (full replication) and on the character for enemies (minimal).

### Player abilities — custom client prediction, not GAS prediction

1. **`ActivateAbility(TriggerEventData)`** — `StoredPayload` (`Origin`, `Yaw`, `ServerSpawnTime`, `Seed`) is
   filled from the trigger data. The server already holds it, so no RPC announces the activation.
2. **`SendFireDataToServer` after `FireDelay`** — optional, ability-specific: sends an updated snapshot when a
   value has changed since activation (current aim yaw, say).

Where `OnFireTargetDataReceived` is used, the server spawns the authoritative projectile from the received
data and hides it from the owning client (`IsNetRelevantFor` false for the owner), which keeps its predicted
one. `ServerSpawnTime` comes off the synchronized server clock (`UGameplayLibrary::GetServerTime`) and
`AGeoProjectile::AdvanceProjectile()` fast-forwards by the elapsed time, so positions match everywhere. For
hold-to-fire the client drives the shot timer and the server fires once per received `FGeoAbilityTargetData`.

**Never use `GetServerTime` for local timing** (charge duration, cooldown UI) — it is a replicated
approximation. Use `GetWorld()->GetTimeSeconds()` or `FPlatformTime::Seconds()`.

Enemy patterns replicate by multicast RPC, spawning deterministically from the same payload.

## Data structures

| Type | Role |
|---|---|
| `FAbilityPayload` | Ability data as `StoredPayload`; also the pattern system's payload |
| `FGeoAbilityTargetData` | Per-shot client→server RPC (`Origin`, `Yaw`, `ServerSpawnTime`, `Seed`; custom `NetSerialize`) |
| `FEffectData` | Polymorphic effect base — damage, heal, shield, generic GE with SetByCaller, single-use damage multiplier, chance-based status |
| `UAbilityInfo` | Data asset of per-class ability arrays; `GetAbilitiesForClass()` returns class + shared |
| `FGameplayAbilityInfo` | One ability entry; carries `EPlayerClass` so one tag can mean a different ability per class |
| `FPlayerClassData` | Per-class runtime data on `APlayableCharacter` — mesh, anim class, default attributes |
| `FHudPlayerParams` | Snapshot of PC, PS, ASC and attribute set handed to the HUD |

**Always read `StoredPayload` fields** rather than the ability's own helpers (`GetAvatarActor()`, …) — the
payload is set by the client and may deliberately differ from ActorInfo.

`SourceOwner` is the actor the shot belongs to: its ASC applies the effects, answers for the team, and owns
what the shot spawns. `SourceAvatar` is what emitted it: origin, montage, self-ignore. They name one ASC for
a player or a boss and two for a turret or a mine, whose `SourceOwner` stays the deployer. They are not
`AActor::Owner`/`Instigator`, and GAS's `Instigator` is this struct's `SourceOwner`.

## Effect application

- Always apply through `UGeoAbilitySystemLibrary::ApplyEffectFromEffectData()`.
- An ability merges its `UEffectDataAsset` references with its inline instances before applying. Inline
  `TArray<TInstancedStruct<FEffectData>>` for effects specific to one ability; `TSoftObjectPtr<UEffectDataAsset>`
  for shared ones.
- Two-pass loop — every `UpdateContextHandle` first, then every `ApplyEffect` — so array order never matters.
- `FSingleUseDamageMultiplierEffectData` sets the multiplier on the context and `UExecCalc_Damage` reads it;
  never read it in `FDamageEffectData::ApplyEffect`.

## Actor pooling

`UGeoActorPoolingSubsystem` (world subsystem): `RequestActor<T>()`, `ReleaseActor()`, `PreSpawn<T>()`.
Pooled actors implement `IGeoPoolableInterface` (`Init()` / `End()`).

## Class-specific abilities

Filter by `EPlayerClass` when activating from an input tag. Never write a separate ability class per player
class — use the `PlayerClass` field on `FGameplayAbilityInfo`.
