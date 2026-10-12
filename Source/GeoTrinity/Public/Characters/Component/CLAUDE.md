# Characters/Component

Components attached to `AGeoCharacter` and subclasses. Visual / audio ones are in `FX/`.

| File | Role |
|---|---|
| `GeoCharacterMovementComponent.h` | Caches base speed/accel; `ApplySpeedMultiplier(float)` driven by `MovementSpeedMultiplier` |
| `GeoDeployableManagerComponent.h` | On players: tracks deployables, slot limits, `ForceExpireAll()`, `OnDeployCountChanged` to HUD |
| `GeoGameFeelComponent.h` | Cosmetic reactions: `FlashOnHit`, `ApplyRecoil` (spring `RecoilRecoverySpeed = 14`), cue rate-limit |
| `ShieldBurstPassiveComponent.h` | Square passive gauge, replicated `GaugeRatio` → shader `GlowGauge` |
| `FX/` | `GeoBeamVFXComponent`, `GeoFXComponent`, `GeoIndicatorComponent`, `GeoLoopSoundComponent`, `GeoDeploySatelliteComponent` |

## Deployable manager
- Limit = global `MaxDeployables = 3` or per-class `DeployableSlots` (0 = unlimited).
- `HasReachMaxLimit` checks class CDO `IsUnlimitedDeploy()` first (agrees on every machine, no replication).
- Always can deploy when `bDestroyOldestWhenLimitReached` (oldest expires on register).
- `SetDeployableInfinitCount` / `RemoveDeployableSlot` only to lift a normally limited class (reload shower).
- `ForceExpireAll()` on EndPlay and class switch.

## Game feel
- Never drives logic. `IsDamageCueAvailable()` / `IsHealCueAvailable()` are consumed by the ExecCalcs when `bLimitGameplayCue` is set.

## Shield-burst component
- Added while `GeoShieldBurstPassiveAbility` is active; `SetGaugeRatio` server-only, replicates via `OnRep_GaugeRatio`.
- Material changes go through `APlayableCharacter::SetBodyMaterial`.
