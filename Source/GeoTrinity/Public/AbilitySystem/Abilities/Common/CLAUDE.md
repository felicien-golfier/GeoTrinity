# Abilities/Common

Abilities shared across all player classes.

---

## `GeoDashAbility.h` — movement dash
Available to all classes.

- Uses `UAbilityTask_ApplyRootMotionMoveToForce` (GAS task) — replicates the root motion source by ID from the server and reconciles it through the CMC saved-move system
- **Not `LaunchCharacter()`** — avoids position-correction artifacts from CMC vs direct launch
- **Not a hand-rolled source + local timer** — the task owns the single authoritative timeout, so client and server end off the same replicated `OnTimedOut`. A client-only wall-clock timer could fire before the server's (started ~half-RTT later), remove the source, and force `MOVE_Walking` mid-dash, producing a correction that cut the dash short on the client.
- `DashDistance = 500 cm`, `DashDuration = 0.2s`
- `OnTimedOut` / `OnTimedOutAndDestinationReached` → `OnDashFinished` ends the ability; the task removes its own source on end
- Direction derived from `StoredPayload.Yaw` at activation time; start is the avatar's current location on each machine (the task ignores any passed start)

---

## `GeoDeployAbility.h` — hold-to-charge deployable deployer
Extends `GeoProjectileAbility`. All deploy abilities (walls, turrets, healing zones) extend this. `InstancedPerActor` — the stack system keeps per-instance runtime state.

- Hold: charges up (uses `ChargeForFireDelay` FireMode)
- Release: fires `ADeployableSpawnerProjectile` that travels and spawns the deployable on ground impact
- Deploy distance (0..1 charge ratio) encoded in `StoredPayload.Seed` as integer cm value
- Deploy distance lerped by charge ratio between `UGameDataSettings::MinDeployDistance` / `MaxDeployDistance` (project-wide, not per-ability)
- `DeployableActorClass` — what to spawn on impact
- `GetDeployableActorClass()` — public accessor; used by `AGeoHUD::GetDeployCountForAbility` to match the ability to the correct deployable-manager slot
- `Params` (`FDeployableDataParams`) — `LifeDrainMaxDuration`, `Size` passed to the deployable

Deploy charge UI: ability calls `PlayableCharacter::ShowDeployChargeGauge(Self)` on activation and `HideDeployChargeGauge()` on end.

### Charge/stack gating (client-predicted)
Deployment is gated by charges, **not** the live-deployable count. `UGeoDeployableManagerComponent` still tracks live deployables (for size effects / recall), but its `MaxDeployables` no longer blocks activation.

- `MaxStacks` (EditDefaultsOnly, per-ability, default 3) — max charges. `GetCurrentStacks()` / `GetMaxStacks()` are the UI hooks.
- `CommitBehaviour = DoNotAutoCommit`. Each activation spends one stack **after** `Super::ActivateAbility` succeeds (guarded on `IsActive()` — Super calls `EndAbility` if it bails on cost, so a failed activation spends nothing). Spending from full also starts the refill clock immediately.
- `CanActivateAbility` gates on `CurrentStacks > 0` (plus the base death check); the manager's `CanDeploy` alive-count check is gone.
- **The ability's Cooldown GE is repurposed as the single shared refill clock.** `CheckCooldown()` is overridden to always return `true` so the cooldown tag never blocks activation (the stack count is the gate) — `ShouldIgnoreCooldowns` is wrong here, it would stop the GE applying at all. The cooldown still drives the ability-bar sweep via the unchanged `GetCooldownTimeRemainingAndDuration`.
- Refill: `OnGiveAbility` registers a `RegisterGameplayTagEvent` on the cooldown GE's granted tag and inits `CurrentStacks = MaxStacks`; `OnRemoveAbility` unbinds. On the tag's removed edge (cooldown expired, `NewCount == 0`) a stack is added; while still `< MaxStacks` the **server** (`GeoLib::IsServer`) re-arms the cooldown for the next stack. Re-arm is authority-only so the client never applies a non-predicted local cooldown GE that would double against the replicated one — the client refills off its predicted first cooldown and off each replicated server-armed cooldown. Model: one stack back per cooldown, one timer at a time.
- Stack-refilled sound: fired locally (`InvokeGameplayCueEvent`, `IsLocallyControlled()`-gated) on the same removed edge. Cue tag is `UGameDataSettings::GenericGameplayCueSoundTag` (shared generic-sound cue, project-wide); the sound identifier is the native `Event.Sound.DeployableAvailable` tag, hardcoded and passed via `CueParams.AggregatedSourceTags` for the generic cue's `GeoSoundRow` lookup.
