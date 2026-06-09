# Abilities/Damaging

Projectile-firing ability base classes.

---

## `GeoProjectileAbility.h` — single-shot projectile

**Extend this for new single-shot abilities.**

- Cost committed **once at activation** (one activation = one shot)
- `Fire()` → client spawns predicted projectile
- `OnFireTargetDataReceived()` → server spawns authoritative projectile
- `SpawnProjectile()` — virtual override point for custom projectile initialization
- `SpawnProjectilesUsingTarget()` — spawns one projectile per direction from `GetTargetDirections()`
- `GetTargetDirections()` — returns directions based on target mode (Forward, AllPlayers, etc.)

---

## `GeoAutomaticFireAbility.h` — hold-to-fire loop (abstract)

**Extend this for new hold-to-fire abilities. Override `ExecuteShot()` — it is pure virtual.**

### How it works
- `ActivateAbility` schedules `Fire()` on a repeating timer driven by `FireDelay`
- `Fire()` runs **client-only** per shot — spawns predicted projectile, sends RPC
- Server receives each shot via `OnFireTargetDataReceived()` (no server-side timer)
- `CommitAbilityCost()` called **per shot on both client and server** — ability ends automatically when cost runs out (e.g. ammo depleted)

### Per-shot animation
- Fire montage plays continuously
- Section index cycles Fire1 → Fire2 → … tracked on ASC via `GetFireSectionIndex()`
- Play rate adjusted so each section matches `FireDelay`

### Game feel fields
- `FireCameraShakeClass` — screen shake per shot
- `RecoilDistance` (cm) — recoil kick via `UGeoGameFeelComponent::ApplyRecoil()`
- `FireGameplayCueTag` — GC tag fired per shot

### Ability-bar sweep
Has no real GAS cooldown, so overrides `GetCooldownTimeRemainingAndDuration` to report the per-shot fire-delay timer (`FireTriggerTimerHandle` remaining / `GetFireDelay()`). The ability-bar slot sweep therefore fills/depletes once per shot instead of staying grayed for the whole hold.

---

## `GeoAutomaticProjectileAbility.h` — concrete auto-fire

Extends `UGeoAutomaticFireAbility`. Implements `ExecuteShot()` to spawn projectiles using `ProjectileClass`.
No further extension needed for basic projectile auto-fire — configure via `ProjectileClass` and effect data in BP.
