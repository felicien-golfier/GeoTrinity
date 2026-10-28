# AbilitySystem/Lib

| File | Role |
|---|---|
| `GeoAbilitySystemLibrary.h` | Static GAS helpers (`GeoASLib`) |
| `GeoGameplayTags.h` | Native tag constants |

## GeoASLib
- **Effects**: `ApplyEffectFromEffectData(Array, SourceASC, TargetASC, Level, Seed, AbilityTag)` — two passes (all `UpdateContextHandle`, then all `ApplyEffect`); fresh context per call.
- **Deployables**: `StartSpawnDeployable` (call `InitInteractable` before finish), `FillDeployableData`, `FinishSpawnDeployable`, `FullySpawnDeployable`.
  - `FullySpawnDeployable` is the only spawn funnel: (Class, Payload, EffectData, Params, Transform) or (Class, pre-filled `FDeployableData` subclass, Transform, World) for pickups and self-owned zones.
  - Self-owned: `Data.Owner` null → spawned in `World` with no owner or instigator, then `Owner` = `Instigator` = the deployable before `InitInteractable`, so its own ASC is the effect source (an encounter's zone). `World` is only read when `Owner` is null.
  - `SpawnDeployableFromAbility(DeployerASC, Location, StartHealthRatio = 1, DeployDistance = -1)`: the deployer's own deploy ability spawns its deployable again — its class, effects and params (`UGeoDeployAbility::MakeSpawnParams(ASC, DeployDistance)`: gems applied, Leverage when `DeployDistance >= 0`), fresh seed, payload from its spec. Overridable: start health ratio and the distance Leverage reads; a new override is a new argument. Spends no charge, throws no projectile. Null when the ASC holds no deploy ability.
- **Projectiles** (pool-aware): `FullySpawnProjectile`, `StartSpawnProjectile` / `FinishSpawnProjectile`; both take only `FExternalProjectileParams` (applied via `ApplyProjectileParams`); `AdvanceProjectile(Elapsed)` compensates lag.
- **Targeting**: `GetTargetDirections(World, Target, Yaw, Origin)`, the one aim code of every projectile ability and pattern; `EProjectileTarget`: `Forward` (caster yaw), `AllPlayers` (live players), `Boss` (live boss of `AGeoArena::GetFightingArena`, none outside a fight).
  - Rule: a new way to aim a projectile = a new `EProjectileTarget` value handled in `GetTargetDirections`, choosable by every projectile ability and pattern (`Target` property); never aim code of its own in an ability, pattern or firer.
- **Agents**: `GetInteractableActors(World, SourceTeam, AttitudeBitmask, …)`, `GetInteractableActorsInLine(…)` (2D beam segment), `IsTeamAttitudeAligned`. Expired deployables (`!IsActive()`, lingering until delayed destroy, e.g. fallen into the void) are never returned.
- `ETargetOverlapMode` (default `Automatic`): `Enemy` source → `CenterOnly`, others → `IncludeRadius`; line half-width always kept.
- **Cues**: `ExecuteLocalGameplayCue(ASC, Tag, Params)` — this machine only, for cosmetics from logic already running everywhere.
- **Context**: `GetStatusTag` / `SetStatusTag`; scoped fields via `FEffectData::UpdateContextHandle`.
- **Abilities**: `GetAbilityCDO<T>(ASC, Tag)`, `GetGrantedAbility<T>(ASC)`.

## GeoGameplayTags
- Namespaces: `InputTag.*` (Basic/Deployable/Special/Reload/Dash), `Ability.*`, `PlayerClass.*`, `Status.*`, `TargetPoint.*`, `Arena.*`, `AI.*`.
- Tags added to `Config/Tags/GeoGameplayTags.ini` need an editor restart to resolve in Python.
