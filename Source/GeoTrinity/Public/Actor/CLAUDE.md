# Actor

World actors: projectiles, deployables, turret, triggers.

## Subfolders
| Folder | Contents |
|---|---|
| `Projectile/` | `GeoProjectile` base + `GeoPooledProjectile`, `GeoShieldBurstProjectile`, `DeployableSpawnerProjectile` |
| `Deployable/` | `GeoDeployableBase` + `GeoMine`, `GeoHealingZone`, `GeoBuffPickup` |
| `Turret/` | `GeoTurret` — Triangle's deployable turret |

## Other Files
- `GeoClassChangeTrigger.h` — volume trigger for runtime class switching
- `GeoInteractableActor.h` — base for world actors that receive `FInteractableActorData`. Key fields: `Owner` (actor whose ASC drives GAS/team checks), `Instigator` (origin avatar for world position/direction — may differ from Owner). Always use `Owner` for ASC lookups; use `Instigator` for cue params and spatial calculations.

See each subfolder's `CLAUDE.md` for full details.
