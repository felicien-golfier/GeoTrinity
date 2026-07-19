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

**The charge pool IS the Cooldown GE's stack count — the ability stores no counter.** `GetCurrentStacks()` returns `StackLimitCount - ASC->GetGameplayEffectCount(CooldownGE)`, `GetMaxStacks()` returns `StackLimitCount`. Both are derived reads, so client and server cannot disagree: the stack count is replicated, the client predicts the spend off the activation prediction key, and GAS reconciles the predicted stack on catch-up (`OnPredictiveGameplayEffectStackCaughtUp`) and in `PostReplicatedChange`.

- `CommitBehaviour = DoNotAutoCommit`. Each activation spends one charge by applying one stack of the Cooldown GE, **after** `Super::ActivateAbility` succeeds (guarded on `IsActive()` — Super calls `EndAbility` if it bails on cost, so a failed activation spends nothing).
- `CanActivateAbility` gates on `GetCurrentStacks() > 0` (plus the base death check); the manager's `CanDeploy` alive-count check is gone.
- `CheckCooldown()` always returns `true` so the cooldown tag never blocks activation (the charge count is the gate) — `ShouldIgnoreCooldowns` is wrong here, it would stop the GE applying at all. The cooldown still drives the ability-bar sweep via the unchanged `GetCooldownTimeRemainingAndDuration`.
- **Refill is the GE's own expiry, not code.** `StackDurationRefreshPolicy = NeverRefresh` means spending a second charge does *not* reset the running timer; `StackExpirationPolicy = RemoveSingleStackAndRefreshDuration` hands back exactly one stack on expiry and restarts a full-duration timer. One `FActiveGameplayEffect`, one timer — charges refill sequentially, never in parallel.
- **Do not hand-roll refills off the cooldown tag's removed edge.** That was the previous design and it was broken on clients: the server re-armed the next cooldown inside the removal callback, so remove+add coalesced into one replicated delta and the client's tag never reached 0. Clients refilled exactly once (on the final cooldown, where the server stops re-arming). A client cannot self-re-arm either — `OnCooldownTagChanged` runs from a timer with no scoped prediction key, so `CommitAbilityCooldown` fails `HasAuthorityOrPredictionKey` and silently no-ops.
- Charge-refilled sound: bound to the cooldown tag with `EGameplayTagEventType::AnyCountChange` — `FActiveGameplayEffect::PostReplicatedChange` → `OnStackCountChange` → `NotifyTagMap_StackCountChange` fires it on clients when a stack replicates away. The delegate's `NewCount` is the tag's *presence*, not the pool, so the handler re-reads `GetCurrentStacks()` and compares against `LastKnownStacks` (local cosmetic state only). Fired locally (`GeoASLib::ExecuteLocalGameplayCue`, `IsLocallyControlled()`-gated). Cue tag is `UGameDataSettings::GenericGameplayCueSoundTag` (shared generic-sound cue, project-wide); the sound identifier is the native `Event.Sound.DeployableAvailable` tag, hardcoded and passed via `CueParams.AggregatedSourceTags` for the generic cue's `GeoSoundRow` lookup.

**`GE_CD_Special` is the deploy charge pool** (`Content/AbilitySystem/GameplayEffects/Cooldown/`). It is shared by all three deploy abilities — `GA_DeployHealingZone`, `GA_Square_Special_Mine`, `GA_LaunchTurret` — and by nothing else; each class grants exactly one, so the effect count is unambiguous. Required config: `StackingType = AggregateByTarget`, `StackLimitCount = 3` (the pool size, and the reason the ability has no `MaxStacks` property), `StackDurationRefreshPolicy = NeverRefresh`, `StackExpirationPolicy = RemoveSingleStackAndRefreshDuration`, plus its existing `HasDuration` (the refill time).
Because the GE is shared, pool size and refill time are **the same for all three deploy abilities**. Never reuse `GE_CD_Special` on a non-deploy ability, and never grant one class two deploy abilities — either would pool their charges together. To vary either per ability, split it into one cooldown GE per deploy ability.
