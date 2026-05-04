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
- `GeoInteractableActor.h` — base for world actors that receive `FInteractableActorData`

See each subfolder's `CLAUDE.md` for full details.
