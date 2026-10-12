# Actor

World actors.

| Folder / file | Holds |
|---|---|
| `Arena/` | One boss encounter: arena, hex arena, barrier, volume → `Arena/CLAUDE.md` |
| `Buttons/` | Floor pads, teleporter, class-change trigger |
| `Deployable/` | `AGeoDeployableBase` and every deployable (wall, turret, mine, pillar, bomb, healing zone, buff pickup, effect zone) |
| `Projectile/` | Projectile base, pooled, shield burst, deployable spawner |
| `GeoInteractableActor.h` | Base of actors receiving `FInteractableActorData` |
| `GeoTargetPoint.h` | Tagged point (`TargetPoint.*` purpose + `Arena.*` arena tag) |

## GeoInteractableActor
- `Owner` = ASC / GAS / team-check source; `Instigator` = origin avatar for cue params and spatial maths. They differ for minions.
- ASC lookups always through `Owner`.

## Target points
- Found with `GeoLib::GetTargetPoints(World, PurposeTag, ArenaTag)` — needs both tags.
- A new arena only needs its own `Arena.*` tag.
