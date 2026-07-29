# Characters/Component

Character components attached to `AGeoCharacter` and its subclasses.

## `GeoCharacterMovementComponent.h`
Extends `UCharacterMovementComponent`; caches base speed/accel on `OnRegister`.
- `ApplySpeedMultiplier(float)` — scales `MaxWalkSpeed`/`MaxAcceleration` relative to cached base, driven by the `MovementSpeedMultiplier` attribute.

## `GeoDeployableManagerComponent.h`
On `APlayableCharacter`. Tracks active deployables, enforces max count (global `MaxDeployables=3`, or per-class caps via `DeployableSlots` — 0 = unlimited).
- `HasReachMaxLimit` first checks the class CDO's `AGeoDeployableBase::IsUnlimitedDeploy()` (default true) and short-circuits to "not at max" — this is a class-level property so every machine agrees without needing `DeployableSlots` to replicate (it doesn't). `AGeoBuffPickup` is the one class that overrides it to false, since the reload buff shower relies on the count limit to evict the oldest uncollected pickup.
- `CanDeploy`/`HasReachMaxLimit` — true always when `bDestroyOldestWhenLimitReached` (oldest expires on register instead of blocking).
- `SetDeployableInfinitCount(Class)`/`RemoveDeployableSlot(Class)` — only still needed to dynamically lift/restore a normally-limited class's cap (the reload buff loot shower); anything unlimited by default doesn't need it.
- `ForceExpireAll()` — called on character EndPlay and class switch.
- `OnDeployCountChanged` — broadcast to HUD/UI on count change.

## `GeoGameFeelComponent.h`
Cosmetic-only reactions on all characters (never drives game logic).
- `FlashOnHit()`, `ApplyRecoil(float)` (springs back at `RecoilRecoverySpeed=14`).
- `IsDamageCueAvailable()`/`IsHealCueAvailable()` — rate-limit; consumed by `ExecCalc_Damage`/`ExecCalc_Heal` when `bLimitGameplayCue` is set — don't call from effect call sites.

## `GeoBeamVFXComponent.h`
Replicated beam-VFX holder, dynamically added by a beam ability on the server in `OnGiveAbility`/destroyed in `OnRemoveAbility` — lives as long as the ability is granted, toggled per activation. Generic, reusable by any beam ability.
- `SetBeamState(bActive, HalfWidth, Length, bIsIndicator=false, IndicatorLifetime=0)` — writes replicated `FBeamVFXState`; server write replicates to all, owning client may also call for lag-free local visuals. `IndicatorLifetime` only matters while `bIsIndicator` — pushed to the indicator system's `Lifetime` user param (`UGeoChannelBeamAbility::ActivateAbility` passes `GetFireDelay()`, so the telegraph animation times out exactly when the beam fires).
- Each rendering machine spawns a local non-replicated `UNiagaraComponent` attached to the owner's **root**. On clients `BeginPlay` runs **before** the initial `BeamSystem` value lands, so creation happens in the `BeamSystem` RepNotify (must end with `ApplyBeamState()` — `OnRep_BeamState` fires first per declaration order and no-ops while Niagara component is null).
- `SetBeamColor` replicates `COND_None` (not `COND_InitialOnly` — dynamically added components miss the initial bunch).
- `BeamSystem` must be authored **local-space pointing +X**.
- **Windup preview**: `IndicatorSystem` (the shared Ray Zone Indicator niagara) is loaded once in `CreateNiagaraComponent` from `UGameDataSettings::BeamPreviewSystem` — one project-wide asset, no per-component/per-BP configuration, and (being identical on every machine already) it never needs to replicate, unlike `BeamSystem`. Only `FBeamVFXState::bIsIndicator` replicates; `ApplyBeamState` swaps the one `NiagaraComponent`'s asset between `IndicatorSystem`/`BeamSystem` on transition via the shared `GeoNiagaraParams::ApplySwappableAsset` helper (also used by `UBeamPattern` — see `Tool/CLAUDE.md`), which reassigns the asset only when it actually differs (reassigning resets the system, which is exactly the desired hand-off look). `UGeoChannelBeamAbility::ActivateAbility` turns the preview on for the fire-delay windup; the first `Tick()` after `Fire()` turns it off implicitly (its `SetBeamState` call defaults `bIsIndicator` to false).

## `ShieldBurstPassiveComponent.h`
Dynamically added to Square while `GeoShieldBurstPassiveAbility` is active; removed when it ends.
- `SetGaugeRatio(float)` — **server-only**; replicates via `OnRep_GaugeRatio` → `OnGaugeRatioChanged` BP event.
- Drives shader param `"GlowGauge"` on slot-0 dynamic material instance. The MID is discarded on any raw slot-0 material set — must go through `APlayableCharacter::SetBodyMaterial`, which re-calls `InitializeMaterialInstances()`.
