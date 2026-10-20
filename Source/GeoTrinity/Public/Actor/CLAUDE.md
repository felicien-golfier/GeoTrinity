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
| `GeoCat.h` | `AGeoCat`: the Tutorial Cat in the world (`SKM_Cat` + `ABP_Cat`); orders: walk, roam, play a montage |

## GeoInteractableActor
- `Owner` = ASC / GAS / team-check source; `Instigator` = origin avatar for cue params and spatial maths. They differ for minions.
- ASC lookups always through `Owner`.

## Target points
- Found with `GeoLib::GetTargetPoints(World, PurposeTag, ArenaTag)` — needs both tags.
- A new arena only needs its own `Arena.*` tag.

## GeoCat
The Cat's 3D body (`AI/Tutorial/CatAnimation.md` for clips and rig, `Cat.md` for the character); `BP_Cat` sets mesh and AnimBP. Captain / navigator: the cat knows *how* to walk, roam and play a montage; *what, where and when* (points, speed, roam area, which montage, hiding) come from the encounter's Cat actions (`Encounter/Action/Cat/CLAUDE.md`). It knows no clip, no portal, no room.
- Orders (C++): `Walk(Points, Speed, StopDistance)`, `Roam(FGeoCatRoam)`, `PlayMontage(Montage)`. One at a time: a new one ends the current (montage stopped, walk dropped).
- `OnOrderEnded(bool bCompleted)` (native multicast): every order ends through it, completed or replaced, so an awaiting action never hangs. `Roam` ends only when replaced.
- No order = idle: it stands and turns toward the first local pawn (`bFacePlayer`), so the idle reads as talking to you; never while a montage plays (a clip effect on a socket must not swing).
- `PlayMontage` completes when the montage starts blending out, not when it ends: an order or a hide given then lands before the blend back to locomotion shows (a clip ending below the plane would rise again). No montage / anim instance: completes at once, so headless tests and a missing asset never stall an encounter.
- Visibility is not the cat's: placed hidden (engine `bHidden`, "Actor Hidden In Game"), shown and hidden by `GeoSetActorHiddenAction`. Hidden = no tick work.
- Locomotion is kinematic, in `Tick` -> `AdvanceMove(DeltaSeconds)` (public for tests): straight segments at the order's speed, no collision, no navmesh, Z kept. Yaw turns toward travel at `TurnRate` (the mesh faces +X).
- `Walk`: the points in order; `StopDistance` > 0 shortens the last segment by it and turns the cat to face the last point on arrival.
- `Roam`: random spot inside `Area`'s bounds inset by `Margin`, walk at `Speed`, pause `Pause` (min / max, random), repeat.
- `Speed` on the AnimBP (0 idle, 1 walk) is written by the cat each tick (property by name on the anim instance), eased at `SpeedBlendRate`; the AnimBP picks the clip (`CatAnimation.md`).
- Not replicated: it lives on the solo tutorial host, where the encounter and the player's view share one process (`AI/Tutorial/Flow.md`, "Maps"); same rule as the Cat line.
