# AbilitySystem/Lib

| File | Role |
|---|---|
| `GeoAbilitySystemLibrary.h` | Static GAS helpers (`GeoASLib`) |
| `GeoGameplayTags.h` | Native tag constants |
| `GeoTarget.h` | `FGeoTarget` (mode + its params), `EGeoTarget`, `FGeoTargetContext`, `FGeoTargetPick` |

## GeoASLib
- **Effects**: `ApplyEffectFromEffectData(Array, SourceASC, TargetASC, Level, Seed, AbilityTag)` — two passes (all `UpdateContextHandle`, then all `ApplyEffect`); fresh context per call.
- **Deployables**: `StartSpawnDeployable` (call `InitInteractable` before finish), `FillDeployableData`, `FinishSpawnDeployable`, `FullySpawnDeployable`.
  - `FullySpawnDeployable` is the only spawn funnel: (Class, Payload, EffectData, Params, Transform) or (Class, pre-filled `FDeployableData` subclass, Transform, World) for pickups and self-owned zones.
  - Self-owned: `Data.Owner` null → spawned in `World` with no owner or instigator, then `Owner` = `Instigator` = the deployable before `InitInteractable`, so its own ASC is the effect source (an encounter's zone). `World` is only read when `Owner` is null.
  - `SpawnDeployableFromAbility(DeployerASC, Location, StartHealthRatio = 1, DeployDistance = -1)`: the deployer's own deploy ability spawns its deployable again — its class, effects and params (`UGeoDeployAbility::MakeSpawnParams(ASC, DeployDistance)`: gems applied, Leverage when `DeployDistance >= 0`), fresh seed, payload from its spec. Overridable: start health ratio and the distance Leverage reads; a new override is a new argument. Spends no charge, throws no projectile. Null when the ASC holds no deploy ability.
- **Projectiles** (pool-aware): `FullySpawnProjectile`, `StartSpawnProjectile` / `FinishSpawnProjectile`; both take only `FExternalProjectileParams` (applied via `ApplyProjectileParams`); `AdvanceProjectile(Elapsed)` compensates lag.
- **Targeting**: "Targeting" below.
- **Agents**: `GetInteractableActors(World, SourceTeam, AttitudeBitmask, …)`, `GetInteractableActorsInLine(…)` (2D beam segment), `IsTeamAttitudeAligned`. Expired deployables (`!IsActive()`, lingering until delayed destroy, e.g. fallen into the void) are never returned.
- `ETargetOverlapMode` (default `Automatic`): `Enemy` source → `CenterOnly`, others → `IncludeRadius`; line half-width always kept.
- **Cues**: `ExecuteLocalGameplayCue(ASC, Tag, Params)` — this machine only, for cosmetics from logic already running everywhere.
- **Context**: `GetStatusTag` / `SetStatusTag`; scoped fields via `FEffectData::UpdateContextHandle`.
- **Abilities**: `GetAbilityCDO<T>(ASC, Tag)`, `GetGrantedAbility<T>(ASC)`.

## Targeting
The one targeting code of every ability: projectiles, salves, beams, zones, pillars, adds, waves. Never aim or placement code of its own in an ability, pattern or firer.
- Everything is a location first; a direction = location − origin, zero length = the context's yaw.
- `FGeoTarget` = `Mode` (`EGeoTarget`, Python name `GeoTargetMode`) + the params of every mode, shown only for theirs (`EditCondition`). One per ability (`Target`, on `UGeoGameplayAbility`; `Abilities/Base/CLAUDE.md`, "Target"). A second need on one ability (where to fire from) = a second `FGeoTarget` there, read through the same functions; never a parallel enum.
- `FGeoTargetContext`: `Caster`, `Origin`, `Yaw`, `Seed`, `ArenaTag` (the room; pattern abilities only). Built by the ability (`MakeTargetContext`), never by a firer.
- `PickTargets(Target, Context)` → `TArray<FGeoTargetPick>`: the one code choosing, seeded from `Context.Seed`. A pick = `Actor` + `Offset` (follows that actor) and `Location` (kept once the actor is gone, so a death mid-cast still lands).
- `ResolvePicks(Picks)` → locations, at the moment of firing. `GetTargetLocations(Target, Context)` = pick then resolve at once; `GetTargetDirections(Target, Context, Origin)` on top of it.
- No target = empty result = nothing fires. No caster fallback; `Forward` always has one.
- A new way of choosing = an `EGeoTarget` value, its params in `FGeoTarget`, a case in `PickTargets`; at once choosable by every ability.
- Single-output consumers (beam, wave) given several use the first.

| `EGeoTarget` | Params | Picks |
|---|---|---|
| `Forward` (default) | `Distance` | `Origin` + yaw × `Distance`; 0 = the origin itself |
| `AllPlayers` | — | Each live player |
| `RandomPlayers` | `Count`, `ExtraBelowHealth`, `MinScatterRadius`, `MaxScatterRadius` | Live players from a seeded start. Total = `Count` + one per `ExtraBelowHealth` ratio above the caster's health ratio, never the number of players; one per player, those beyond the alive count scattered around a player within the radii |
| `Boss` | — | Live boss of `AGeoArena::GetFightingArena`; none outside a fight |
| `TargetPoint` | `TargetPointTag` | First point tagged it in `Context.ArenaTag` (`GeoLib::GetTargetPoints`); none, with an ensure, when no room or no point |
| `ArenaCenter` | — | The caster's arena's location (`AGeoArena::GetArenaOfBoss`), no actor to follow; none, with an ensure, when it has no arena |

- `ExtraBelowHealth`: the condition (the caster's health ratio) is fixed in code; a ramp-up system owning the conditions (steps) comes later, inside this targeting.

## GeoGameplayTags
- Namespaces: `InputTag.*` (Basic/Deployable/Special/Reload/Dash), `Ability.*`, `PlayerClass.*`, `Status.*`, `TargetPoint.*`, `Arena.*`, `AI.*`.
- Tags added to `Config/Tags/GeoGameplayTags.ini` need an editor restart to resolve in Python.
