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
- `GeoArenaBarrier.h` — replicated barrier that blocks the arena entrance; opens/closes via `SetClosed(bool)` (server-only); animated actors lerp full `FTransform` between `FightOnTransform` and `FightOffTransform` over `CommitFightTime` (read from `AGeoGameState`). Editor helpers: `CaptureFightOn/OffTransforms`, `SetToFightOn/OffTransforms`.

See each subfolder's `CLAUDE.md` for full details.
