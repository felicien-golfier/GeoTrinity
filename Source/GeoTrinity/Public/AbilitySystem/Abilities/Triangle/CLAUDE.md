# Abilities/Triangle

Triangle (DPS) kit. Basic attack = `UGeoAutomaticProjectileAbility` with ammo cost (`UCharacterAttributeSet::Ammo`).

| File | Role |
|---|---|
| `GeoReloadAbility.h` | Restores ammo, spawns a buff pickup |
| `GeoRecallTurretAbility.h` | Recalls all turrets, effects on players along each turret→player line |

## Reload
- `AmmoRestoreEffect` restores ammo; `BuffPickupClass` (`AGeoBuffPickup`) spawned between `MinSpawnRadius`/`MaxSpawnRadius`.
- `GetReachableSpawnOffset()` pulls the offset back on a blocked path (`PickupRadius` clearance).
- `PowerScale = MissingAmmo / MaxAmmo`.
- `BuffColors` palette parallel to merged `BuffEffectDataAssets`; `GetColorForIndex`, static `GetColorForAmmo` (HUD preview, via ability CDO).

## Recall turret
- `UGeoDeployableManagerComponent::RecallAll()`; `BlinkBonusEffect` for turrets already blinking.
- `OverlapAttitude` (default `HostileOrNeutral`) picks affected team; `FindTargets` uses `GetInteractableActorsInLine` with `LineHalfWidth`.
