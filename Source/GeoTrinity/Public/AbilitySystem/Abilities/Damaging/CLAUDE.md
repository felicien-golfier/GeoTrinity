# Abilities/Damaging

Projectile-firing ability base classes.

---

## `GeoProjectileAbility.h` — single-shot projectile
**Extend this for new single-shot abilities.**
- Cost committed once at activation
- `Fire()` → client spawns predicted projectile; `OnFireTargetDataReceived()` → server spawns authoritative
- `SpawnProjectile()` — virtual override point; uses `StartSpawnProjectile`+`FinishSpawnProjectile` for mid-spawn property injection
- `ProjectileParams` (`FGeoProjectileParams`, see `Actor/Projectile/CLAUDE.md`) — one member holding the projectile class + its distance/speed/radius/color overrides; the struct overload of `StartSpawnProjectile` applies it. Descriptions resolve `{DistanceSpan}`/`{ProjectileSpeed}` off `ProjectileParams`
- `SpawnProjectilesUsingTarget()` — one projectile per direction from `GetTargetDirections()` (Forward/AllPlayers/etc.)

## `GeoAutomaticFireAbility.h` — hold-to-fire loop (abstract)
**Extend this for new hold-to-fire abilities. Override `ExecuteShot()` (pure virtual).**
- `ActivateAbility` schedules `Fire()` on a repeating timer (`FireDelay`)
- `Fire()` runs client-only per shot; server receives each shot via `OnFireTargetDataReceived()` (no server-side timer)
- Cost committed per shot: `Fire()` commits for locally controlled player; `SendFireDataToServer` skips the RPC when `IsServer()` (host's own shots never reach `OnFireTargetDataReceived` on that machine); `OnFireTargetDataReceived` always commits (only reachable for remote clients' shots processed server-side)
- Fire montage cycles Fire1→Fire2→… via ASC-tracked section index, play rate matched to `FireDelay`
- Game feel: `FireCameraShakeClass`, `RecoilDistance` (via `UGeoGameFeelComponent::ApplyRecoil()`), `FireGameplayCueTag`
- No real GAS cooldown — overrides `GetCooldownTimeRemainingAndDuration` to report the per-shot fire-delay timer, so the ability-bar sweep fills/depletes per shot instead of staying grayed for the whole hold

## `GeoAutomaticProjectileAbility.h` — concrete auto-fire
Extends `UGeoAutomaticFireAbility`, implements `ExecuteShot()` spawning via `ProjectileParams.ProjectileClass`. Configure via `ProjectileParams` + effect data in BP; same `FGeoProjectileParams` overrides as `GeoProjectileAbility`.
