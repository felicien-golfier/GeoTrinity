# Abilities/Common

Shared by every player class.

| File | Role |
|---|---|
| `GeoDashAbility.h` | Dash via `UAbilityTask_ApplyRootMotionMoveToForce` (500 cm / 0.2 s) |
| `GeoDeployAbility.h` | Hold-to-charge deployer; base of every deploy (wall, turret, healing zone, mine) |
| `GeoTriggeredEffectAbility.h` | Applies its effect data to its owner when a watched ability hits (`HitTriggerTags`) or activates (`ActivationTriggerTags`); data-only "on hit / on cast, gain X" |
| `GeoDeployTargetCue.h` | Looping landing marker: follows `GetPendingDeployLocation()` while charging, then sits on the projectile |
| `GeoZoneIndicatorCue.h` | Round zone telegraph via `UGeoIndicatorComponent::SpawnIndicator`; `RawMagnitude` = radius, `Normal.X` = lifetime, `Normal.Y` = elapsed |

## Dash
- Direction from `StoredPayload.Yaw`; start = avatar location on each machine.
- Root-motion task owns the timeout (replicated `OnTimedOut`).

## Deploy
- Extends `GeoProjectileAbility`; release fires `ADeployableSpawnerProjectile`, which spawns `DeployableActorClass` on impact.
- Distance = charge ratio encoded in `StoredPayload.Seed` as integer cm, lerped `MinDeployDistance`..`MaxDeployDistance` (`GameDataSettings`).
- UI: `ShowDeployChargeGauge(Self)` / `HideDeployChargeGauge()`.

### Charges
- Charge pool = stack count of the Cooldown GE; no ability-side counter.
- `GetCurrentStacks() = StackLimitCount - GetGameplayEffectCount(CooldownGE)` (derived, same on client and server).
- `CommitBehaviour = DoNotAutoCommit`; one stack applied after `Super::ActivateAbility` succeeds (guard `IsActive()`).
- `CanActivateAbility` gates on stacks > 0; `CheckCooldown()` always true.
- Refill = the GE's own expiry: `NeverRefresh` + `RemoveSingleStackAndRefreshDuration`.
- `GE_CD_Deployable` (`Content/AbilitySystem/GameplayEffects/Cooldown/`): `AggregateByTarget`, limit 3; shared by healing zone, mine, turret only. A different pool size/refill = its own GE.
- Charge-refilled sound: cooldown tag `AnyCountChange` → `OnStackCountChange`; local cue `GenericGameplayCueSoundTag` with `Event.Sound.DeployableAvailable`.
