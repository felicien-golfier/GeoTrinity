# Abilities/Common

Abilities shared across all player classes.

---

## `GeoDashAbility.h` — movement dash
Available to all classes.

- Uses `FRootMotionSource_MoveToForce` — integrated into `UCharacterMovementComponent`'s saved-move system
- **Not `LaunchCharacter()`** — avoids position-correction artifacts from CMC vs direct launch
- `DashDistance = 500 cm`, `DashDuration = 0.2s`
- Root motion source ID tracked on the ability, cleared on `EndAbility`
- Direction derived from `StoredPayload.Yaw` at activation time

---

## `GeoDeployAbility.h` — hold-to-charge deployable deployer
Extends `GeoProjectileAbility`. All deploy abilities (walls, turrets, healing zones) extend this.

- Hold: charges up (uses `ChargeForFireDelay` FireMode)
- Release: fires `ADeployableSpawnerProjectile` that travels and spawns the deployable on ground impact
- Deploy distance (0..1 charge ratio) encoded in `StoredPayload.Seed` as integer cm value
- `MinDeployDistance` / `MaxDeployDistance` — lerped by charge ratio
- `DeployableActorClass` — what to spawn on impact
- `GetDeployableActorClass()` — public accessor; used by `AGeoHUD::GetDeployCountForAbility` to match the ability to the correct deployable-manager slot
- `Params` (`FDeployableDataParams`) — `LifeDrainMaxDuration`, `Size` passed to the deployable

Deploy charge UI: ability calls `PlayableCharacter::ShowDeployChargeGauge(Self)` on activation and `HideDeployChargeGauge()` on end.
