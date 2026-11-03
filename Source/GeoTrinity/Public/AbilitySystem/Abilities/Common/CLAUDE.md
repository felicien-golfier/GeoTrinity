# Abilities/Common

Shared by every player class.

| File | Role |
|---|---|
| `GeoDashAbility.h` | Dash via `UAbilityTask_ApplyRootMotionMoveToForce` (500 cm / 0.2 s) |
| `GeoDeployAbility.h` | Hold-to-charge deployer; base of every deploy (wall, turret, healing zone, mine) |
| `GeoPlaceDeployableAbility.h` | Places a deployable at its `Target`, no projectile (enemy zones) |
| `GeoTriggeredEffectAbility.h` | Applies its effect data to its owner when a watched ability hits (`HitTriggerTags`) or activates (`ActivationTriggerTags`); data-only "on hit / on cast, gain X" |
| `GeoZoneIndicatorCue.h` | `UGeoZoneIndicatorCue`: the cue that launches a zone-layer shape (`UGeoIndicatorComponent::SpawnIndicator`). Cue Blueprint defaults: `Shape` (`Round` / `Ray`), `Layer`, `FillMode`. Params: `RawMagnitude` radius (ray: length), `Normal.X` lifetime, `Normal.Y` already elapsed; colours from the cue; attached to `SourceObject`'s root when an actor. Any Blueprint cue draws the same through `SpawnZoneShape` |
| `GeoDeployTargetCue.h` | Looping landing marker: follows `GetPendingDeployLocation()` while charging, then sits on the projectile |

## Dash
- Direction from `StoredPayload.Yaw`; start = avatar location on each machine.
- Root-motion task owns the timeout (replicated `OnTimedOut`).

## PlaceDeployable
- `UGeoPlaceDeployableAbility` (`UGeoGameplayAbility` child, `ServerOnly`, no projectile, no charges, no HUD): the deploy without the throw, for an enemy's zone. Own `DeployableActorClass`, `Params` (`FDeployableDataParams`) and `Size` (`FScalableFloat`, read at the ability's level so a difficulty column scales it; 0 keeps `Params.Size`). Unrelated to `UGeoDeployAbility`'s properties: nothing moves between them.
- Shape of `UGeoSpawnOnTileAbility` (`../Boss/CLAUDE.md`): `Fire` → per location of its `Target` (`Base/CLAUDE.md`, "Target"; `Self` for a zone on its caster) `GeoASLib::FullySpawnDeployable(Class, StoredPayload, GetEffectDataArray(), Params with Size, Transform)` → `EndAbility`. Instigator = the caster, so the deployable sits in its manager and dies with it. The effects (zone damage per second) are the ability's own `EffectDataInstances`.
- No gems applied: `Params` as authored. A caster with gems later calls the same gem adjustment `UGeoDeployAbility::MakeSpawnParams` makes (read through `GeoASLib::GetStatValue(ASC, Attr, 1.f)`, 1 = none); never a case for enemies.
- Does not touch `GeoASLib::SpawnDeployableFromAbility`: that one re-spawns a deploy ability's own deployable (gem passives).

## Deploy
- Extends `GeoProjectileAbility`; release fires `ADeployableSpawnerProjectile`, which spawns `DeployableActorClass` on impact.
- Distance = charge ratio encoded in `StoredPayload.Seed` as integer cm, lerped `MinDeployDistance`..`MaxDeployDistance` (`GameDataSettings`).
- UI: `ShowDeployChargeGauge(Self)` / `HideDeployChargeGauge()`.
- Owns what it deploys (class, `Params`), never the spawning of copies: `MakeSpawnParams(ASC, DeployDistance = -1)` = `Params` with the deployer's gems (blink, drain, health), Leverage when `DeployDistance >= 0`, and `DeployDistance` stored in the params; the throw passes its charged distance, `SpawnProjectile` sets the result on the spawner projectile. Placing one without a throw = `GeoASLib::SpawnDeployableFromAbility`.

### Charges
- Charge pool = stack count of the Cooldown GE; no ability-side counter.
- `GetCurrentStacks(ASC) = GetMaxStacks(ASC) - GetGameplayEffectCount(CooldownGE)` (derived, same on client and server; reads only the given ASC, so the CDO answers too).
- `CommitBehaviour = DoNotAutoCommit`; one stack applied after `Super::ActivateAbility` succeeds (guard `IsActive()`).
- `CanActivateAbility` gates on stacks > 0; `CheckCooldown()` always true.
- `IsReady` override: stacks > 0 (the cooldown GE runs while charges remain, so the default rule would unlight the slot).
- Refill = the GE's own expiry: `NeverRefresh` + `RemoveSingleStackAndRefreshDuration`.
- `GE_CD_Deployable` (`Content/AbilitySystem/GameplayEffects/Cooldown/`): `AggregateByTarget`, limit 3; shared by healing zone, mine, turret only. A different pool size/refill = its own GE.
- Charge-refilled sound: cooldown tag `AnyCountChange` → `OnStackCountChange`; local cue `GenericGameplayCueSoundTag` with `Event.Sound.DeployableAvailable`.
