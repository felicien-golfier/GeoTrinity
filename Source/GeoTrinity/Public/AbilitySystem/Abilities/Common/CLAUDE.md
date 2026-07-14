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
Each deploy use is banked as a charge. Activating consumes one charge immediately (`ConsumeCharge()`, called from `ActivateAbility` — skipped if `Super::ActivateAbility` already ended the ability, e.g. a failed cost commit). The server recharges one charge at a time in the background (`ChargeRechargeTime` seconds each, sequential — only one charge regenerates even if several are missing) up to `MaxCharges`. `CanActivateAbility` additionally gates on `CurrentCharges > 0` and on `MinTimeBetweenDeploys` having elapsed since the last activation (lets a player with several banked charges spend them only one at a time, not all in one frame). This whole system is orthogonal to `UGeoDeployableManagerComponent::CanDeploy()` (still checked, unchanged) — that component caps how many deployables may be alive in the world at once; charges cap how often the ability can be pressed.

- `MaxCharges`, `ChargeRechargeTime`, `MinTimeBetweenDeploys` — tunable per ability BP subclass (Wall/Turret/HealingZone each get independent values)
- `CurrentCharges` / `NextChargeReadyServerTime` — replicated `COND_OwnerOnly` (ability sets `InstancingPolicy = InstancedPerActor` + `ReplicationPolicy = ReplicateYes` in its constructor for this). Only the server runs the recharge `FTimerHandle`; the owning client's copy is prediction-only (decremented locally in `ActivateAbility`, corrected by replication) — same pattern as cost/cooldown commit elsewhere in the base class
- `GetCurrentCharges()` / `GetMaxCharges()` / `GetChargeRechargeTimeRemainingAndDuration()` — public accessors read by `AGeoHUD::GetDeployChargesForAbility` for the ability-bar's charges badge
- Does **not** use GAS's native `CooldownGameplayEffectClass`/`CheckCooldown` at all — `GetCooldownTimeRemainingAndDuration` is overridden to report `MinTimeBetweenDeploys` remaining/duration instead, so the ability-bar sweep flashes briefly once per deploy (the small delay) rather than reflecting an unrelated or unconfigured cooldown effect
- Recharge timing uses `GeoLib::GetServerTime` (network-approximated, correct on both server and client) since it tracks a server-driven background event — NOT `GetWorld()->GetTimeSeconds()`, which is reserved for the local-only `MinTimeBetweenDeploys` check (input-feel timing, per `Tool/CLAUDE.md`)
