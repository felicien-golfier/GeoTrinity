# DeployableStartHealth — pending

Goal: a deployable can start at a fraction of its health with max health unchanged (Split's copy first, other passives' later), and every deployable spawn goes through one library funnel.
Docs written: `Source/GeoTrinity/Public/Actor/Deployable/CLAUDE.md`, `.../AbilitySystem/Lib/CLAUDE.md`, `.../AbilitySystem/Abilities/Common/CLAUDE.md`, `.../AbilitySystem/Abilities/Gem/CLAUDE.md`, `.../Actor/Deployable/BuffPickup/CLAUDE.md`.
Grouping: nothing to fold (no new file).

## Code (to implement)
- [x] `Actor/Deployable/GeoDeployableBase.h` — `FDeployableDataParams::DeployDistance = -1.f` (`UPROPERTY()`, not hand-edited): the throw distance Leverage was computed from.
- [x] `Actor/Deployable/GeoDeployableBase.h/.cpp` — `FDeployableDataParams::StartHealthRatio = 1.f` (not hand-edited, like `HealthMultiplier`); `ApplyHealthMultiplier()` then sets `Health = MaxHealth * clamp(ratio, KINDA_SMALL_NUMBER, 1)` after the multiplier; header comment updated.
- [x] `AbilitySystem/Abilities/Common/GeoDeployAbility.h/.cpp` — public static `MakeSpawnParams(ASC, DeployDistance = -1)` (was private static `ApplyGemsToParams` + Leverage block of `GetGemParams`; Leverage only when `DeployDistance >= 0`; stores `DeployDistance` in the params), public read of `DeployableActorClass` (`GetDeployableActorClass` exists) and effect array; `SpawnProjectile` calls `MakeSpawnParams(ASC, SpawnParams.DistanceSpan)`; remove `GetGemParams` and `SpawnDeployableAt`.
- [x] `AbilitySystem/Lib/GeoAbilitySystemLibrary.h/.cpp` — `SpawnDeployableFromAbility(DeployerASC, Location, StartHealthRatio = 1.f, DeployDistance = -1.f)` (body = old `SpawnDeployableAt`, params from `MakeSpawnParams(ASC, DeployDistance)`, ratio goes to `Params.StartHealthRatio`, not `HealthMultiplier`); `FullySpawnDeployable(Class, FDeployableData&, Transform)` overload, the existing one forwards to it.
- [x] `Gem/GeoSplitPassiveAbility.h/.cpp` — `CopyHealthFraction` → `StartHealthRatio` (0.5, ClampMin 0.01, ClampMax 1, `EditDefaultsOnly` (UHT forbids BlueprintReadOnly on private members)); `SplitDeployable` calls `SpawnDeployableFromAbility(HolderASC, location + random offset, StartHealthRatio, Deployable.GetData()->Params.DeployDistance)`; payload / params copy removed.
- [x] `Gem/GeoKinshipPassiveAbility.h/.cpp`, `Gem/GeoWakePassiveAbility.h/.cpp` — same `StartHealthRatio` property (default 1), passed to the call (they were on the default argument).
- [x] `Triangle/GeoReloadAbility.cpp`, `Actor/Arena/GeoArena.cpp` (loot) — `SpawnActorDeferred<AGeoBuffPickup>` replaced by `FullySpawnDeployable(BuffPickupClass, PickupData, Transform)`. Actor `Owner` becomes `Payload.SourceOwner` (reload used the avatar; arena loot already did). Checked by reading: nothing reads a pickup's `AActor::Owner` (logic uses `GetData()->Owner`, unchanged; manager registration uses `Instigator`, unchanged; no RPC, no owner-relevancy); the sole `GetOwner()` comparison is `GeoSacrificeBeamAbility.cpp:181`, on walls only. Re-check after build: reload pickup picked up by the same team, registered in the manager, evicted oldest-first.
- [x] Grep `SpawnActorDeferred<AGeo` for deployable subclasses left outside the funnel.

## Editor / Blueprint (by hand or script)
- [ ] `GA_Core_Split`: if it overrides `CopyHealthFraction`, add `+PropertyRedirects=(OldName="GeoSplitPassiveAbility.CopyHealthFraction",NewName="GeoSplitPassiveAbility.StartHealthRatio")` to `Config/DefaultEngine.ini` (grep of text files found no use; the .uasset is binary).
- [ ] Tune `StartHealthRatio` on `GA_Core_Split` / `GA_Core_Kinship` / `GA_Core_Wake` if wanted (`AI/Python/Ability/gem_core_passives.py` may set it if it should be built, not hand-set).

## Verify
- [ ] Test in `Private/Tests/GeoGemTests.cpp` (lightest layer, `AI/Testing.md`): `FullySpawnDeployable` with `StartHealthRatio = 0.5` → `Health == 0.5 * MaxHealth`, `MaxHealth` equals ratio-1 spawn, `DrainMagnitudePerSecond` equal; `SpawnDeployableFromAbility` ratio reaches the deployable and uses the ability's params; pickup spawned through the funnel registers and keeps its fields.
- [ ] Run `Tools\Run_Tests.bat Gem` headless once built (`GEO_NO_PAUSE=1`, editor closed); big task → at least the Editor target builds.
