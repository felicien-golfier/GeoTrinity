# AbilitySystem/Lib

| File | Role |
|---|---|
| `GeoAbilitySystemLibrary.h` | Static GAS helpers (`GeoASLib`) |
| `GeoGameplayTags.h` | Native tag constants |

## GeoASLib
- **Effects**: `ApplyEffectFromEffectData(Array, SourceASC, TargetASC, Level, Seed, AbilityTag)` — two passes (all `UpdateContextHandle`, then all `ApplyEffect`); fresh context per call.
- **Deployables**: `StartSpawnDeployable` (call `InitInteractable` before finish), `FillDeployableData`, `FinishSpawnDeployable`, `FullySpawnDeployable`.
- **Projectiles** (pool-aware): `FullySpawnProjectile`, `StartSpawnProjectile` / `FinishSpawnProjectile`; both take only `FExternalProjectileParams` (applied via `ApplyProjectileParams`); `AdvanceProjectile(Elapsed)` compensates lag.
- **Targeting**: `GetTargetDirections(Payload, Mode)`.
- **Agents**: `GetInteractableActors(World, SourceTeam, AttitudeBitmask, …)`, `GetInteractableActorsInLine(…)` (2D beam segment), `IsTeamAttitudeAligned`.
- `ETargetOverlapMode` (default `Automatic`): `Enemy` source → `CenterOnly`, others → `IncludeRadius`; line half-width always kept.
- **Cues**: `ExecuteLocalGameplayCue(ASC, Tag, Params)` — this machine only, for cosmetics from logic already running everywhere.
- **Context**: `GetStatusTag` / `SetStatusTag`; scoped fields via `FEffectData::UpdateContextHandle`.
- **Abilities**: `GetAbilityCDO<T>(ASC, Tag)`, `GetGrantedAbility<T>(ASC)`.

## GeoGameplayTags
- Namespaces: `InputTag.*` (Basic/Deployable/Special/Reload/Dash), `Ability.*`, `PlayerClass.*`, `Status.*`, `TargetPoint.*`, `Arena.*`, `AI.*`.
- Tags added to `Config/Tags/GeoGameplayTags.ini` need an editor restart to resolve in Python.
