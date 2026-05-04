# Abilities/Triangle

Triangle (DPS) class abilities. Triangle's basic attack uses `UGeoAutomaticProjectileAbility` with ammo cost (`UCharacterAttributeSet::Ammo`).

---

## `GeoReloadAbility.h` — ammo reload with buff pickup
- Applies `AmmoRestoreEffect` (UGameplayEffect) to restore ammo
- Spawns `BuffPickupClass` (`AGeoBuffPickup`) at a random position between `MinSpawnRadius` and `MaxSpawnRadius` from the character
- `PowerScale = MissingAmmo / MaxAmmo` — pickup buff magnitude scales with how empty the ammo was
- `BuffEffectDataAssets` — array of possible buffs; one is selected randomly using `StoredPayload.Seed`

---

## `GeoRecallTurretAbility.h` — turret recall with effects
- Recalls all deployed `AGeoTurret` actors via `UGeoDeployableManagerComponent::RecallAll()`
- Applies effects to nearby players on recall
- `BlinkBonusEffect` — additional effect applied to turrets that were already **blinking** (near expiry) when recalled
- `OverlapAttitude` — filters which team receives the effects (default: `Hostile`)
