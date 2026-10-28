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
| `GeoGemDrop.h` | `AGeoGemDrop`: a granted gem (3D tier solid) or shard thrown in a fountain, lying until a character steps on it, visual only; sounds on spawn and pickup → `Gem/CLAUDE.md` ("3D gems", "Drop sounds") |
| `GeoCatAnim.h` | `GeoCatAnim::WriteSpeed(Mesh, Speed)`: the by-name `Speed` write on `ABP_Cat`, shared by `AGeoCat` and `AGeoTrainingCat` |
| `GeoCat.h` | `AGeoCat`: the Tutorial Cat in the world (`SKM_Cat` + `ABP_Cat`); orders: walk, roam, play a montage |

## GeoInteractableActor
- `Owner` = ASC / GAS / team-check source; `Instigator` = origin avatar for cue params and spatial maths. They differ for minions.
- ASC lookups always through `Owner`.

## Target points
- Found with `GeoLib::GetTargetPoints(World, PurposeTag, ArenaTag)` — needs both tags.
- A new arena only needs its own `Arena.*` tag.

## GeoCat
The Cat's 3D body (`AI/Tutorial/CatAnimation.md` for clips and rig, `Cat.md` for the character); `BP_Cat` sets mesh and AnimBP. Captain / navigator: the cat knows *how* to walk, roam and play a montage; *what, where and when* (points, speed, roam area, which montage, hiding) come from the encounter's Cat actions (`Encounter/Action/Cat/CLAUDE.md`). It knows no clip, no portal, no room.
- Orders (C++): `Walk(Points, Speed, StopDistance)`, `Roam(FGeoCatRoam)`, `PlayMontage(Montage)`. One at a time: a new one ends the current (montage stopped, walk dropped). `Stop()` ends the current one with no new order (an encounter abort).
- `OnOrderEnded(bool bCompleted)` (native multicast): every order ends through it, completed or replaced, so an awaiting action never hangs. `Roam` ends only when replaced.
- No order = idle: it stands and turns toward the first local pawn (`bFacePlayer`), so the idle reads as talking to you. No turn of any kind while a montage plays (an order's or an idle variant): a montage does not drive the rotation, so the clip must not slide round.
- `PlayMontage` completes when the montage starts blending out, not when it ends: an order or a hide given then lands before the blend back to locomotion shows (a clip ending below the plane would rise again). No montage / anim instance: completes at once, so headless tests and a missing asset never stall an encounter.
- Visibility is not the cat's: placed hidden (engine `bHidden`, "Actor Hidden In Game"), shown and hidden by `GeoSetActorHiddenAction`. Hidden = no tick work.
- Locomotion is kinematic, in `Tick` -> `AdvanceMove(DeltaSeconds)` (public for tests): straight segments at the order's speed, no collision, no navmesh, Z kept. Yaw turns toward travel at `TurnRate` (the mesh faces +X).
- `Walk`: the points in order; `StopDistance` > 0 shortens the last segment by it and turns the cat to face the last point on arrival. On a hidden cat (no tick work, it would never arrive): `ensureMsgf`, the order completes at once, so its action never hangs; show it first.
- `Roam`: random spot inside `Area`'s bounds inset by `Margin`, walk at `Speed`, pause `Pause` (min / max, random), repeat.
- `Speed` on the AnimBP is the cat's real speed in cm/s (0 standing), written each tick (property by name on the anim instance), eased at `SpeedBlendRate` (cm/s per s); the AnimBP picks and blends the clip (`CatAnimation.md`): the order's `Speed` therefore chooses Prowl or Fast.
- Idle variants: while the cat stands (no walking, no montage playing; a roam pause counts) it plays a random montage of `IdleVariants` (set on `BP_Cat`: the five `AM_Cat_Idle_*`), then waits `IdleVariantPause` (min / max s, random) for the next: a countdown run by `AdvanceMove` while standing with no montage playing, so the wait starts when the variant ends or is cut short, and a walk or a montage order holds it and a hidden cat (no tick work) never counts. Starts armed at `BeginPlay`. Draws from a shuffled bag (`GeoLib::DrawFromShuffledBag`): all of them come round before any repeats, never the same twice in a row. Empty list = none. Not an order (it never fires `OnOrderEnded`); any order starts by stopping a playing variant, so a walk or a montage order is never held back by one.
- Not replicated: it lives on the solo tutorial host, where the encounter and the player's view share one process (`AI/Tutorial/Flow.md`, "Maps"); same rule as the Cat line.
