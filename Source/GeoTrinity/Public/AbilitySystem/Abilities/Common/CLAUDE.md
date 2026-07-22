# Abilities/Common

Abilities shared across all player classes.

---

## `GeoDashAbility.h` — movement dash
Available to all classes.
- Uses `UAbilityTask_ApplyRootMotionMoveToForce` (GAS task) — replicates root motion by ID, reconciles via CMC saved-move system
- Not `LaunchCharacter()` (avoids CMC vs direct-launch correction artifacts); not a hand-rolled timer (the task owns the single authoritative timeout so client/server end off the same replicated `OnTimedOut` — a client-only wall-clock timer could fire early, force `MOVE_Walking` mid-dash, and cut the dash short)
- `DashDistance = 500cm`, `DashDuration = 0.2s`
- Direction from `StoredPayload.Yaw` at activation; start = avatar's current location on each machine (task ignores any passed start)

## `GeoDeployAbility.h` — hold-to-charge deployable deployer
Extends `GeoProjectileAbility`. All deploy abilities (walls, turrets, healing zones) extend this. `InstancedPerActor`.

- Hold charges (`ChargeForFireDelay`); release fires `ADeployableSpawnerProjectile` that spawns the deployable on impact
- Deploy distance (0..1 charge ratio) encoded in `StoredPayload.Seed` as integer cm, lerped between `UGameDataSettings::MinDeployDistance`/`MaxDeployDistance` (project-wide)
- `DeployableActorClass` — spawn class; `GetDeployableActorClass()` used by `AGeoHUD::GetDeployCountForAbility`
- `Params` (`FDeployableDataParams`) — `LifeDrainMaxDuration`, `Size`
- UI: ability calls `ShowDeployChargeGauge(Self)`/`HideDeployChargeGauge()`

### Charge/stack gating (client-predicted)
Deployment is gated by charges, not live-deployable count (`UGeoDeployableManagerComponent` still tracks live deployables for size effects/recall, but `MaxDeployables` no longer blocks activation).

**The charge pool IS the Cooldown GE's stack count** — no ability-side counter. `GetCurrentStacks()` = `StackLimitCount - ASC->GetGameplayEffectCount(CooldownGE)`; derived reads, so client/server can't disagree (client predicts the spend off the activation prediction key, GAS reconciles via `OnPredictiveGameplayEffectStackCaughtUp`/`PostReplicatedChange`).

- `CommitBehaviour = DoNotAutoCommit`: each activation applies one Cooldown-GE stack **after** `Super::ActivateAbility` succeeds (guarded on `IsActive()` — a failed activation spends nothing)
- `CanActivateAbility` gates on `GetCurrentStacks() > 0`
- `CheckCooldown()` always `true` — the charge count is the real gate (`ShouldIgnoreCooldowns` would be wrong, it'd stop the GE applying). Cooldown still drives the ability-bar sweep via `GetCooldownTimeRemainingAndDuration`
- **Refill is the GE's own expiry, not code.** `StackDurationRefreshPolicy = NeverRefresh` + `StackExpirationPolicy = RemoveSingleStackAndRefreshDuration` → one `FActiveGameplayEffect`, one timer, charges refill sequentially
- **Do not hand-roll refills off the cooldown tag's removed edge** — previously broken on clients (server re-arming inside the removal callback coalesced remove+add into one replicated delta; client tag never reached 0). A client can't self-re-arm either (`OnCooldownTagChanged` has no scoped prediction key, `CommitAbilityCooldown` fails `HasAuthorityOrPredictionKey`)
- Charge-refilled sound bound to the cooldown tag (`AnyCountChange`) → `OnStackCountChange` re-reads `GetCurrentStacks()` vs `LastKnownStacks` (local cosmetic state), fires `GeoASLib::ExecuteLocalGameplayCue` gated `IsLocallyControlled()`. Cue tag: `UGameDataSettings::GenericGameplayCueSoundTag`; sound id `Event.Sound.DeployableAvailable` via `CueParams.AggregatedSourceTags`

**`GE_CD_Special` is the deploy charge pool** (`Content/AbilitySystem/GameplayEffects/Cooldown/`), shared by `GA_DeployHealingZone`, `GA_Square_Special_Mine`, `GA_LaunchTurret` and nothing else. Config: `StackingType = AggregateByTarget`, `StackLimitCount = 3`, `StackDurationRefreshPolicy = NeverRefresh`, `StackExpirationPolicy = RemoveSingleStackAndRefreshDuration`. Pool size/refill time are the same for all three abilities since the GE is shared — never reuse it on a non-deploy ability, and never grant one class two deploy abilities (would pool charges together). Split into per-ability cooldown GEs to vary either value.
