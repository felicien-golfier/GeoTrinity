# Characters/Component

Character components attached to `AGeoCharacter` and its subclasses.

## `GeoCharacterMovementComponent.h`
Extends `UCharacterMovementComponent`. Caches base speed and acceleration on `BeginPlay`.

- `ApplySpeedMultiplier(float Multiplier)` — scales `MaxWalkSpeed` and `MaxAcceleration` relative to cached base values
- Driven by `MovementSpeedMultiplier` attribute changes from `UCharacterAttributeSet`
- `GetGeoCharacter()` — typed owner access

## `GeoDeployableManagerComponent.h`
Attached to `APlayableCharacter`. Tracks active deployables, enforces max count.

- `MaxDeployables = 3` — configurable per class
- `CanDeploy()` — false when at limit
- `RegisterDeployable(AGeoDeployableBase*)` — registers + binds to destroy callback
- `RecallAll()` — destroys all tracked deployables
- `GetDeployRatio()` — `CurrentCount / MaxDeployables`
- `OnDeployCountChanged` delegate — broadcast to HUD/UI on count change
- `Deployables` array is **replicated**

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
