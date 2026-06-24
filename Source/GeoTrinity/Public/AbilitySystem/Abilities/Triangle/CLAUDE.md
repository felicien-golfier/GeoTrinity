# Abilities/Triangle

Triangle (DPS) class abilities. Triangle's basic attack uses `UGeoAutomaticProjectileAbility` with ammo cost (`UCharacterAttributeSet::Ammo`).

---

## `GeoReloadAbility.h` — ammo reload with buff pickup
- Applies `AmmoRestoreEffect` (UGameplayEffect) to restore ammo
- Spawns `BuffPickupClass` (`AGeoBuffPickup`) at a random position between `MinSpawnRadius` and `MaxSpawnRadius` from the character; uses `GetReachableSpawnOffset()` to pull the offset back when the path is blocked (line-trace on `GeoCharacter` channel), keeping the pickup reachable.
- `PowerScale = MissingAmmo / MaxAmmo` — pickup buff magnitude scales with how empty the ammo was
- `BuffColors` — per-buff color palette, indexed in parallel with the merged `BuffEffectDataAssets` array; the pickup's dynamic material is tinted with the entry matching the chosen buff.
- `GetColorForIndex(int32)` — returns `BuffColors[Index % Num]`, White when palette is empty. Called from the spawned pickup CDO to tint its material.
- `GetColorForAmmo(int32 Ammo)` — `static`; maps ammo count to a color the same way `Fire()` maps it to a buff index (HUD ammo number preview). Looks up the Blueprint-derived reload ability CDO via `AbilityInfo` because it is `static` and has no direct access to the authored `BuffColors` palette.
- `PickupRadius` — half-width clearance kept from a blocking wall when pulling the spawn offset back to a reachable point.

---

## `GeoRecallTurretAbility.h` — turret recall with effects
- Recalls all deployed `AGeoTurret` actors via `UGeoDeployableManagerComponent::RecallAll()`
- Applies effects to nearby players on recall
- `BlinkBonusEffect` — additional effect applied to turrets that were already **blinking** (near expiry) when recalled
- `OverlapAttitude` — filters which team receives the effects (default: `Hostile`)
