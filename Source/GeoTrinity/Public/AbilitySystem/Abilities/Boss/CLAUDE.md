# Abilities/Boss

Server-only boss abilities. Where comes from the activation params (`Base/CLAUDE.md`, "Activation params"), whom from the ability's own `Target` (`Base/CLAUDE.md`, "Target"); only a `RequiresArena()` ability reaches the arena itself, via `AGeoArena::GetArenaOfBoss(StoredPayload.Owner)`, never looked up by class.
A plain `UPatternAbility` + pattern Blueprint needs no class here (tile-carving ray, cone spray, sweep beam, pillars, a summon's telegraph): where it lands is its `Target` (`Base/CLAUDE.md`, "Pattern target"), what lands is its pattern.

| File | Role |
|---|---|
| `GeoPeriodicPatternAbility.h` | Never ends: burst timing only; each `USalvePattern` launch is one salve at its `Target`, picked and stamped right before it leaves, then schedules the next |
| `GeoTileBombAbility.h` | Draws a live player, spawns `AGeoBombZone` attached to them; not a pattern (replicated deployable) |
| `GeoSpawnOnTileAbility.h` | Turret / mine drop on arena tiles; ServerOnly, not a pattern |
| `GeoSpawnEnemyCharacterAbility.h` | The one ability that spawns an enemy: `EnemyClass` + `Setup` on each `Target` location, into the caster's arena; on demand or as its owner dies |

## Montages
- Bomb and turret have no pattern, so their `AnimMontage` replicates (`Base/CLAUDE.md`, "Montage ownership"). Authored in the `Cast` slot group, apart from the pattern montages' group, so a cast never cuts a running pattern montage.

## GeoSpawnOnTileAbility
- `DeployableClass`, `DeployableParams`, `SpawnCount`; `LifeDrainMaxDuration` arms a mine fuse, 0 for turrets.
- `bSpawnOnHealthScaledRing`: ring radius = `HealthRatio * GridRadius`; stateless so it survives boss reset.
- Tiles via `GetRandomAliveTiles` (distinct, unoccupied); `Fire` marks `SetTileOccupant`.
- Deployables unlimited by default; arena recalls them when their tile dies.

## Tile bomb
- Random pick: `FRandomStream(Seed)` + `RandHelper`.
- Two phases live on `AGeoBombZone` (`Actor/Deployable/BombZone`).

## Summon
- No summon class: two separate abilities, in order, in the caster's tree (states `Summon`, then `Land`). Dummies and allies are replicated enemy actors, never synchronised.
- `Summon`: plain `UPatternAbility` + `UZonePattern` Blueprint, telegraph and hit only; `Target` picks the place (`RandomPlayers`, `Count` > 1 = several dummies). Its `FireDelay` (a plain float, the same at every level) holds the caster still and is the telegraph's length; the hit's numbers are its `EffectDataInstances`; the pattern's `AnimMontage` is the caster's cosmetic summon animation (the Pillar drop clip).
- `Land`: `UGeoSpawnEnemyCharacterAbility`, `Target` `PatternTargets` = that pattern class (`Lib/CLAUDE.md`, "Targeting"), so the dummy lands on the telegraphed spot; the hit never touches it, it spawns after. Its ability tag is a sibling of the summon's, never a child: `STTask_FireAbility` activates child tags too.
- Force-stop: boss death and wipe end the tree. Between a wipe and its reset the tree may still reach `Land`; `ResetAdds` removes that dummy.
- Each add owns its fate through its setup's `AbilityTags` (`Characters/CLAUDE.md`, "Enemy setup"): its zone (`UGeoPlaceDeployableAbility`, passive, `Common/CLAUDE.md`, `Target` `Self`) and its ally (spawn ability, `bSpawnOnOwnerDying`, `Target` `Self`). Casts overlap freely: nothing is shared between them.

## GeoSpawnEnemyCharacterAbility
- Every spawned variant is a Blueprint child holding only data: `EnemyClass`, `Setup` (`FGeoEnemySetup`), `Target`. `ServerOnly`, `InstancedPerActor`, no replication.
- `Fire`: `Arena->SpawnAdd(EnemyClass, location, &Setup)` per `Target` location, then ends. Arena = `GetArenaOfBoss(avatar)`, so a wipe removes the spawn; `RequiresArena()` true.
- Constructor: no general charge time, `FireDelay` 0, so `Fire` runs inside the activation. Needed on death: a timer would be cancelled by `DeathLogic` in the same call.
- `bSpawnOnOwnerDying` (default false): the ability activates itself on its owner's `Event.Character.Dying` (`Characters/CLAUDE.md`, `Death()`). `OnGiveAbility` (server, the per-actor instance) binds `GenericGameplayEventCallbacks`; weak, lives as long as the instance, no unbind. Not a passive: no `Ability.Type.Passive`, nothing stays active while it waits; the Passive tag would fire it at spawn (`ReactivatePassiveAbilities`).
- The owner is still alive during that call, so "a dead owner can never start one" (`Base/CLAUDE.md`) holds, and `Target` `Self` is where it stands.
- Timing: the spawn comes before `bIsDead` and the `OnCharacterDied` broadcast, so a `GeoActorDiedTrigger` counting `Alive` already sees it. The tutorial miniboss's rules filter by actor tag, so it never counts the ally as a dummy; any new rule on summons filters by tag too.
- Destroyed without dying (`DestroyAdds`, wipe, boss death): no event, no spawn.
- Tutorial: `GA_TutorialSpawnDummy` = `BP_Dummy` + summon setup; `GA_TutorialSpawnAlly` = `BP_DummyAlly` + player team, its own tree and the `Tag` the encounter watches.

## GeoPeriodicPatternAbility
- Burst timing only (rounds by health, salves per round by player count); aim is the base's `Target` (constructor default `AllPlayers`), one bullet per target location (`Pattern/Projectile/CLAUDE.md`, "Salve"). No target (boss dead, no fight) → that launch skipped, schedule kept.
- An aim variant is a Blueprint child setting only `Target` (tutorial: `GA_TutorialSalve` for the boss and its summons, its child `GA_TutorialAllySalve` = `Boss` for their allies); the pattern BP's bullets are `OverlapAttitude` hostile only, so each side hits only the other (an ally's bullet never hurts the player). The ally child owns its pattern Blueprint child: head and trail `AllyDamage` colour, and its damage is its own row (`AllyProjectileDamage`).
