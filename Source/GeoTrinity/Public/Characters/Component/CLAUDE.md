# Characters/Component

Character components attached to `AGeoCharacter` and its subclasses.

## `GeoCharacterMovementComponent.h`
Extends `UCharacterMovementComponent`. Caches base speed and acceleration on `OnRegister`.

- `ApplySpeedMultiplier(float Multiplier)` — scales `MaxWalkSpeed` and `MaxAcceleration` relative to cached base values
- Driven by `MovementSpeedMultiplier` attribute changes from `UCharacterAttributeSet`
- `GetGeoCharacter()` — typed owner access

## `GeoDeployableManagerComponent.h`
Attached to `APlayableCharacter`. Tracks active deployables, enforces max count with optional per-class caps.

- `MaxDeployables = 3` — global cap; applies to classes without a `DeployableSlots` entry
- `DeployableSlots` — `TMap<TSubclassOf, int32>`; add an entry to give a class its own independent limit. Set value to 0 for unlimited.
- `CanDeploy(TSubclassOf)` — true if deployment is allowed; always true when the class has `bDestroyOldestWhenLimitReached` set (oldest expires on register instead of blocking). Otherwise delegates to `HasReachMaxLimit`.
- `HasReachMaxLimit(TSubclassOf)` — checks the per-class slot if one exists, otherwise checks the global pool
- `SetDeployableInfinitCount(Class)` — adds `DeployableSlots[Class] = 0`; used by patterns that must spawn an arbitrary number of pillars and by the boss loot shower (`AGeoGameState::SpawnLootBurst`)
- `RemoveDeployableSlot(Class)` — removes the `DeployableSlots` entry, putting the class back on the global `MaxDeployables` pool
- `RegisterDeployable(AGeoDeployableBase*)` — registers + binds to destroy/expire delegates; expires oldest if `bDestroyOldestWhenLimitReached` and slot is full
- `ForceExpireAll()` — immediately expires all tracked deployables; called on character EndPlay and class switch
- `GetDeployables()` / `GetDeployables<T>()` — all live deployables (optionally filtered by class)
- `OnDeployCountChanged` delegate — broadcast to HUD/UI on count change

## `GeoGameFeelComponent.h`
Attached to all characters. Centralizes cosmetic reactions — never drives game logic.

- `FlashOnHit()` — flashes mesh with HitFlashMaterial for HitFlashDuration
- `ApplyRecoil(float Distance)` — kicks mesh backward opposite to yaw, springs back at `RecoilRecoverySpeed = 14`
- `IsDamageCueAvailable()` / `IsHealCueAvailable()` — rate-limit cue triggers; consumed by `ExecCalc_Damage` / `ExecCalc_Heal` when `bLimitGameplayCue` is set on the effect data — don't call from effect call sites
- Auto-discovers owner's first mesh on `BeginPlay` (`TargetMesh`)

## `GeoBeamVFXComponent.h`
Replicated beam-VFX holder, dynamically added by a beam ability on the server in `OnGiveAbility` (`NewObject` + `RegisterComponent`) and destroyed in `OnRemoveAbility` — it exists for as long as the ability is granted, and is toggled on/off per activation. Generic: reusable by any beam ability.

- `SetBeamState(bActive, HalfWidth, Length)` — writes replicated `FBeamVFXState`. The **server** write replicates to everyone; the owning client may also call it for lag-free local visuals
- Each rendering machine (clients + listen host; dedicated server skips) spawns a local non-replicated `UNiagaraComponent` in `BeginPlay`, attached to the owner's **root** so the beam rotates with aim
- `OnRep_BeamState` → `ApplyBeamState()`: `SetActive` + pushes `User.BeamHalfWidth` / `User.BeamLength` (param names editable per BP subclass)
- `BeamSystem` (`UNiagaraSystem`) must be assigned in the BP subclass defaults; author the system **local-space pointing +X**

## `ShieldBurstPassiveComponent.h`
Dynamically added to Square while `GeoShieldBurstPassiveAbility` is active. Removed when ability ends.

- `SetGaugeRatio(float)` — **server-side only**; updates replicated `GaugeRatio`
- `OnGaugeRatioChanged(float)` — `BlueprintImplementableEvent`; fires on all clients via `OnRep_GaugeRatio`
- Drives shader parameter `"GlowGauge"` on slot-0 dynamic material instance (`CharacterMaterialInstance`)
- `Charge()` — plays charge animation when gauge reaches 100%
- `ChargeTime` and `DischargeTime = 0.3f` control animation timing
