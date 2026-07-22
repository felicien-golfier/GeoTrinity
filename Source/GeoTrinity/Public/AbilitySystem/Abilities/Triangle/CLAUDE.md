# Abilities/Triangle

Triangle (DPS) class abilities. Basic attack uses `UGeoAutomaticProjectileAbility` with ammo cost (`UCharacterAttributeSet::Ammo`).

---

## `GeoReloadAbility.h` — ammo reload with buff pickup
- Applies `AmmoRestoreEffect` to restore ammo
- Spawns `BuffPickupClass` (`AGeoBuffPickup`) between `MinSpawnRadius`/`MaxSpawnRadius`; `GetReachableSpawnOffset()` pulls the offset back on a blocked path (line-trace on `GeoCharacter` channel) to keep it reachable
- `PowerScale = MissingAmmo / MaxAmmo` — pickup magnitude scales with how empty ammo was
- `BuffColors` — palette parallel to merged `BuffEffectDataAssets`; pickup's dynamic material tinted to the chosen buff
- `GetColorForIndex(int32)` — `BuffColors[Index % Num]`, White when empty
- `GetColorForAmmo(int32)` — static, maps ammo count to color the same way `Fire()` maps it to a buff index (HUD preview); looks up the reload ability CDO via `AbilityInfo` since it's static
- `PickupRadius` — clearance kept from a blocking wall when pulling the spawn offset back

## `GeoRecallTurretAbility.h` — turret recall with effects
- Recalls all deployed `AGeoTurret` via `UGeoDeployableManagerComponent::RecallAll()`
- Applies effects to nearby players on recall; `BlinkBonusEffect` extra effect for turrets already blinking (near expiry)
- `OverlapAttitude` filters which team receives effects (default `HostileOrNeutral`)
- `FindTargets` uses `GeoASLib::GetInteractableActorsInLine` along turret→player segment; `LineHalfWidth` sets width
