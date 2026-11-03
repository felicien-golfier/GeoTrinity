# Abilities/Boss

Server-only boss abilities. Where comes from the activation params (`Base/CLAUDE.md`, "Activation params"), whom from the ability's own `Target` (`Base/CLAUDE.md`, "Target"); only a `RequiresArena()` ability reaches the arena itself, via `AGeoArena::GetArenaOfBoss(StoredPayload.Owner)`, never looked up by class.
A plain `UPatternAbility` + pattern Blueprint needs no class here (tile-carving ray, cone spray, sweep beam, pillars, summons): where it lands is its `Target` (`Base/CLAUDE.md`, "Pattern target"), what lands is its pattern.

| File | Role |
|---|---|
| `GeoPeriodicPatternAbility.h` | Never ends: burst timing only; each `USalvePattern` launch is one salve at its `Target`, picked and stamped right before it leaves, then schedules the next |
| `GeoTileBombAbility.h` | Draws a live player, spawns `AGeoBombZone` attached to them; not a pattern (replicated deployable) |
| `GeoSpawnOnTileAbility.h` | Turret / mine drop on arena tiles; ServerOnly, not a pattern |
| `GeoSpawnOnDeathAbility.h` | Passive of an add (`AutoAbilityTags`): spawns an ally where its owner dies (`Summon`) |

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
- No summon class: the cast is a plain `UPatternAbility` + `USpawnAddPattern` (launch, spawn, end; `Target` `Count` > 1 = several dummies per cast). The tree's Summon state completes on the ability's end like any other. Dummies and allies are replicated enemy actors, never synchronised.
- Each add owns its fate through passives in `AddSetup.AbilityTags` (`Characters/CLAUDE.md`, "Enemy setup"): its zone (`UGeoPlaceDeployableAbility`, `Common/CLAUDE.md`, `Target` `Self`) and `UGeoSpawnOnDeathAbility`. Casts overlap freely: nothing is shared between them.

## GeoSpawnOnDeathAbility
- Passive of an add (`Ability.Type.Passive`, server only), activated at the enemy's `InitGAS`; never ends on its own. `AllyClass`, `AllySetup` live on it (the ally's data).
- Activation binds `AGeoGameState::OnCharacterDied`, filtered on its own avatar. On that death: `Arena->SpawnAdd(AllyClass, the avatar's transform, &AllySetup)`, the arena = the avatar's `Arena`, so the next wipe removes the ally too. `Death()` broadcasts before `DeathLogic` cancels the abilities, so the delegate always runs first.
- Timing: the ally spawns inside the `OnCharacterDied` broadcast, not after it. Listeners run in reverse bind order, so this one (bound last) runs before encounter triggers bound earlier; a `GeoActorDiedTrigger` counting `Alive` during that call already sees the ally. The tutorial miniboss's rules filter by actor tag, so it never counts the ally as a dummy; any new rule on summons filters by tag too.
- Destroyed without dying (`DestroyAdds`, wipe, boss death): no death, no ally; `EndAbility` unbinds.
- Tutorial: `AllyClass` = `BP_DummyAlly`, `AllySetup` = player team, its own tree and `Tag` the encounter watches.

## GeoPeriodicPatternAbility
- Burst timing only (rounds by health, salves per round by player count); aim is the base's `Target` (constructor default `AllPlayers`), one bullet per target location (`Pattern/Projectile/CLAUDE.md`, "Salve"). No target (boss dead, no fight) → that launch skipped, schedule kept.
- An aim variant is a Blueprint child setting only `Target` (tutorial: `GA_TutorialSalve` for the boss and its summons, its child `GA_TutorialAllySalve` = `Boss` for their allies); the pattern BP's bullets are `OverlapAttitude` hostile only, so each side hits only the other (an ally's bullet never hurts the player). The ally child owns its pattern Blueprint child: head and trail `AllyDamage` colour, and its damage is its own row (`AllyProjectileDamage`).
