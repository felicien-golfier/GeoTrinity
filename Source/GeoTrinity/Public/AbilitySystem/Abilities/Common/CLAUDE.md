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

Deploy charge UI (hold-to-aim gauge): ability calls `PlayableCharacter::ShowDeployChargeGauge(Self)` on activation and `HideDeployChargeGauge()` on end. Not to be confused with the deploy **charges/stacks** system below (different meaning of "charge": aim-hold ratio vs. banked uses).

### Deploy charges (stacks) — usage economy, independent of `UGeoDeployableManagerComponent`'s alive-count cap
Each deploy use is banked as a charge. Activating consumes one charge immediately (`ConsumeCharge()`, called from `ActivateAbility` — skipped if `Super::ActivateAbility` already ended the ability, e.g. a failed cost commit). One charge recharges in the background at a time (`ChargeRechargeTime` seconds each, sequential — only one charge regenerates even if several are missing) up to `MaxCharges`. `CanActivateAbility` additionally gates on `CurrentCharges > 0` and on `MinTimeBetweenDeploys` having elapsed since the last activation (lets a player with several banked charges spend them only one at a time, not all in one frame). This whole system is orthogonal to `UGeoDeployableManagerComponent::CanDeploy()` (still checked, unchanged) — that component caps how many deployables may be alive in the world at once; charges cap how often the ability can be pressed.

- `MaxCharges`, `ChargeRechargeTime`, `MinTimeBetweenDeploys` — tunable per ability BP subclass (Wall/Turret/HealingZone each get independent values)
- `CurrentCharges` / `NextChargeReadyWorldTime` / the recharge `FTimerHandle` are **not replicated**. `ActivateAbility` already runs symmetrically on both the predicting client and the server (same as the base class's cost/cooldown commit), so both sides independently run the same recharge countdown off their own local `GetWorld()->GetTimeSeconds()` and naturally stay in sync — no round trip needed. The server is still the sole authority on whether an activation actually goes through; a mispredicting client's activation is rejected/rolled back the same way a cost or cooldown misprediction already is
- `GetCurrentCharges()` / `GetMaxCharges()` / `GetChargeRechargeTimeRemainingAndDuration()` — public accessors read by `AGeoHUD::GetDeployChargesForAbility` for the ability-bar's charges badge (reads whichever machine's HUD is asking — no cross-machine sync involved)
- Does **not** use GAS's native `CooldownGameplayEffectClass`/`CheckCooldown` at all — `GetCooldownTimeRemainingAndDuration` is overridden to report `MinTimeBetweenDeploys` remaining/duration instead, so the ability-bar sweep flashes briefly once per deploy (the small delay) rather than reflecting an unrelated or unconfigured cooldown effect
