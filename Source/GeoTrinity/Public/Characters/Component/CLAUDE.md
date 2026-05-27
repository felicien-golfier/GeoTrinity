# Characters/Component

Character components attached to `AGeoCharacter` and its subclasses.

## `GeoCharacterMovementComponent.h`
Extends `UCharacterMovementComponent`. Caches base speed and acceleration on `BeginPlay`.

- `ApplySpeedMultiplier(float Multiplier)` — scales `MaxWalkSpeed` and `MaxAcceleration` relative to cached base values
- Driven by `MovementSpeedMultiplier` attribute changes from `UCharacterAttributeSet`
- `GetGeoCharacter()` — typed owner access

## `GeoDeployableManagerComponent.h`
Attached to `APlayableCharacter`. Tracks active deployables, enforces max count with optional per-class caps.

- `MaxDeployables = 3` — global cap; applies to classes without a `DeployableSlots` entry
- `DeployableSlots` — `TMap<TSubclassOf, int32>`; add an entry to give a class its own independent limit. Set value to 0 for unlimited.
- `CanDeploy(TSubclassOf opt)` — checks the per-class slot if one exists, otherwise checks the global pool
- `SetDeployableInfinitCount(Class)` — adds `DeployableSlots[Class] = 0`; used by patterns that must spawn an arbitrary number of pillars
- `RegisterDeployable(AGeoDeployableBase*)` — registers + binds to destroy/expire delegates
- `ExpireAll()` — recalls/destroys all tracked deployables
- `GetDeployables()` / `GetDeployables<T>()` — all live deployables (optionally filtered by class)
- `GetDeployRatio()` — `CurrentCount / MaxDeployables`
- `OnDeployCountChanged` delegate — broadcast to HUD/UI on count change

## `GeoGameFeelComponent.h`
Attached to all characters. Centralizes cosmetic reactions — never drives game logic.

- `FlashOnHit()` — flashes mesh with HitFlashMaterial for HitFlashDuration
- `ApplyRecoil(float Distance)` — kicks mesh backward opposite to yaw, springs back at `RecoilRecoverySpeed = 14`
- `IsDamageCueAvailable()` / `IsHealCueAvailable()` — rate-limit cue triggers; check before firing a Gameplay Cue
- Auto-discovers owner's first mesh on `BeginPlay` (`TargetMesh`)

## `ShieldBurstPassiveComponent.h`
Dynamically added to Square while `GeoShieldBurstPassiveAbility` is active. Removed when ability ends.

- `SetGaugeRatio(float)` — **server-side only**; updates replicated `GaugeRatio`
- `OnGaugeRatioChanged(float)` — `BlueprintImplementableEvent`; fires on all clients via `OnRep_GaugeRatio`
- Drives shader parameter `"GlowGauge"` on slot-0 dynamic material instance (`CharacterMaterialInstance`)
- `Charge()` — plays charge animation when gauge reaches 100%
- `ChargeTime` and `DischargeTime = 0.3f` control animation timing
