# Actor

World actors: projectiles, deployables, turret, triggers.

## Subfolders
| Folder | Contents |
|---|---|
| `Projectile/` | `GeoProjectile` base + `GeoPooledProjectile`, `GeoShieldBurstProjectile`, `DeployableSpawnerProjectile` |
| `Deployable/` | `GeoDeployableBase` + `GeoWall`, `GeoHealingZone`, `GeoBuffPickup` |
| `Turret/` | `GeoTurret` — Triangle's deployable turret |

## Other Files
- `GeoClassChangeTrigger.h` — volume trigger for runtime class switching
- `GeoMatchStateButton.h` — overlap button; requests a configurable match-state transition (`StartMatch` / `WaitingToStart` / `WaitingPostMatch`) from `AGeoGameMode`. Server-only, mirrors `GeoClassChangeTrigger`.
- `GeoInteractableActor.h` — base for world actors that receive `FInteractableActorData`. Key fields: `Owner` (actor whose ASC drives GAS/team checks), `Instigator` (origin avatar for world position/direction — may differ from Owner). Always use `Owner` for ASC lookups; use `Instigator` for cue params and spatial calculations.
- `GeoEffectZone.h` — editor-placeable area that applies a configurable `TArray<TInstancedStruct<FEffectData>>` to actors inside its capsule. Extends `AGeoInteractableActor` (owns its own ASC), so it needs no spawner/ability — configure `Team`, `AttitudeBitmask`, `EffectDataArray`, `Radius`, `Level` in the Details panel. Self-inits GAS in `BeginPlay`; overlap delegates (`OnBeginOverlap`/`OnEndOverlap`) maintain `ActorsInZone`. `Tick()` re-applies heal/damage effects (magnitude treated as a per-second rate) to every tracked actor; other effect types are applied once on entry and removed on exit. Build heal zones (heal effect + Friendly attitude) or damage zones (damage effect + configurable `AttitudeBitmask`) purely from data.
- `GeoArenaBarrier.h` — replicated barrier that blocks the arena entrance; opens/closes via `SetClosed(bool)` (server-only); animated actors lerp full `FTransform` between `FightOnTransform` and `FightOffTransform` over `CommitFightTime` (read from `AGeoGameState`). Editor helpers: `CaptureFightOn/OffTransforms`, `SetToFightOn/OffTransforms`. The lerp is driven by the actor's own `Tick`, enabled from `OnRep_bIsClosed` (the single state-change seam, called manually by `SetClosed` on the server since OnReps don't fire on the authority) and disabled once `LerpAlpha` settles at its target. Drive it *only* that way — the previous `SetTimerForNextTick` chain had a tautological stop condition (`LerpAlpha` is clamped to `[0,1]`, so `bIsClosed && LerpAlpha <= 1 || !bIsClosed && LerpAlpha >= 0` is always true), so no chain ever terminated and every redundant entry point spawned a parallel one that each advanced the alpha once per frame. The barrier closed N× too fast, N = live chain count, accumulating with every open/close. `OnBarrierStateChanged` is a pure `BlueprintImplementableEvent` for VFX only — never hang lerp driving off it, or a BP override that forgets the parent call freezes the barrier.

See each subfolder's `CLAUDE.md` for full details.
