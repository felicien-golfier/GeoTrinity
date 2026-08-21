# VFX Knowledge Base

---

## Asset Duplication via Python

The only reliable method — use `load_object` + `AssetTools.duplicate_asset`:
```python
import unreal
obj = unreal.load_object(None, "/Game/VFX/Abilities/Triangle/TurretRecall/NS_Square_RurretRecall_Beam.NS_Square_RurretRecall_Beam")
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
result = asset_tools.duplicate_asset("NS_GenericLaser", "/Game/VFX/Generic/Niagara", obj)
if result:
    unreal.EditorAssetLibrary.save_asset(result.get_path_name())
```

`EditorAssetLibrary.duplicate_asset(src, dst)` silently returns `None` for NiagaraSystem assets — do not use it.

Raw `shutil.copy2()` is also useless — the internal UE package name stays as the source name and the asset registry won't recognize it.

## Asset Paths

Use `a.package_name` and `a.asset_name` — `a.object_path` no longer exists in UE5.

Search for assets by class:
```python
import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
filter = unreal.ARFilter(class_names=["NiagaraSystem"], recursive_paths=True, package_paths=["/Game"])
for a in ar.get_assets(filter):
    print(a.package_name)
```

## Project Asset Paths

| Asset | Path |
|-------|------|
| Turret recall beam | `/Game/VFX/Abilities/Triangle/TurretRecall/NS_Square_RurretRecall_Beam` |
| Generic laser | `/Game/VFX/Generic/Niagara/NS_GenericLaser` |
| Glow material instance | `/Game/VFX/Generic/Materials/MatInstances/MI_Glow01` |
| Unlit particle material | `/Game/VFX/Generic/Materials/M_Particle_Unlit_Advanced` |
| Zone indicator (ring + hard fill) | `/Game/VFX/Generic/Materials/M_ZoneIndicator` |
| Zone indicator ray (bar, center→edges) | `/Game/VFX/Generic/Materials/M_ZoneIndicatorRay` |
| Pulse circle (outline ring + inward pulsing fill) | `/Game/VFX/Generic/Materials/M_PulseCircle` |
| Pulse beam (outline frame + pulse running to target) | `/Game/VFX/Generic/Materials/M_PulseBeam` |
| Clock-wipe mask function (remaining-life readout) | `/Game/VFX/Generic/Materials/Functions/MF_DurationWipe` |
| Moira beam niagara (`Beam_Length`/`Beam_Width`/`Color`) | `/Game/VFX/Assets/NS_Cirlce_MoiraBeam` |

Note: The turret recall asset has a typo — "Rurret" not "Turret".

## Mesh Renderer Material Usage

A material a mesh renderer draws with has its mesh-particle usage flag set the first time the system renders, which
dirties that material asset. Save it along with the system — without the flag the effect falls back to the default
material outside the editor.

## Custom Primitive Data

Slot 0 is the deployable duration wipe and is the only slot in use. `AGeoDeployableBase::Tick` writes
`1 - GetDurationPercent()` onto every visual mesh; `M_PulseCircle`'s `DurationSpent` parameter reads it back
with `bUseCustomPrimitiveData`. This replaces a per-actor dynamic material instance — one shared material
serves every zone in the level.

The value is the fraction **spent**, never the fraction remaining, and inverting it is not cosmetic: custom
primitive data nobody writes reads 0, so "remaining" would render every unwritten primitive as fully drained.
`MI_PulseCircle`'s sibling instance `PulseCircleInst` on `BP_DamageZone` is exactly that case — a
`GeoEffectZone`, not a deployable, so nothing ever writes its slot.

Claiming a new slot means picking an unused index and adding it here — the material addresses the slot by
index, so nothing warns when two features collide on one.

`M_PulseBeam` carries the same readout under the same `DurationSpent` name but as a plain scalar parameter,
because it is driven from a Niagara emitter: a renderer binds material parameters, and custom primitive data
belongs to a primitive component, which a sprite is not.

## MCP Tool Schema Constraint

The Go MCP server only forwards JSON fields defined in the tool's schema. Extra fields are silently dropped. To pass custom data to a C++ route, add a dedicated operation in `NiagaraRoutes.cpp` that takes only `system_path` and hardcodes the logic.

## MCP Niagara Operations (C++)

Operations in `NiagaraRoutes.cpp` (`/api/niagara/ops`): `spawn_system`, `set_parameter`, `get_system_info`,
`add_emitter`, `remove_emitter`, `activate`, `deactivate`. Any other name is rejected as unknown.

### `get_system_info`
Lists emitters and user parameters. Uses `GetUserParameters()` (not `ReadParameterVariables()`) to skip stale redirect aliases.

### Module stacks
These routes do not reach a module stack. Adding modules, setting static switches, writing input values and
nesting dynamic inputs go through the editor utility shim — see `AI/MCP/MCP_Niagara.md`.

## Niagara Parameter Store

Every `User.*` name the game code writes is declared once in `Source/GeoTrinity/Public/Tool/GeoNiagaraParams.h` (namespace `GeoNiagaraParams`). Adding a user param to a system means adding it there too — never spell the name at a call site. Renaming one in the Niagara asset means renaming it there, and nothing else will tell you: an unmatched name is silently ignored and the system keeps its authored default.

`RenameParameter()` leaves a stale redirect alias in `UserParameterRedirects`. Always use `GetUserParameters()` to display clean params — it skips aliases.

`RecreateRedirections()` is not exported from the Niagara DLL — causes LNK2019. Do not call it.

## SavePackage

Always set `SaveFlags = SAVE_NoError` to avoid crashes in `SerializeLocMetadataValue`:
```cpp
FSavePackageArgs SaveArgs;
SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
SaveArgs.SaveFlags = SAVE_NoError;
UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs);
```

## search_assets Assert Fix

`FTopLevelAssetPath(shortName)` asserts on short class names. Always check for `.` first:
```cpp
if (ClassFilter.Contains(TEXT("."))) {
    ClassPath = FTopLevelAssetPath(ClassFilter);
} else {
    for (TObjectIterator<UClass> It; It; ++It) {
        if (It->GetName() == ClassFilter) {
            ClassPath = FTopLevelAssetPath(It->GetPackage()->GetFName(), It->GetFName());
            break;
        }
    }
}
```

## Niagara Graph: LerpPosition Ribbon Wiring

The turret recall beam uses:
- Spawn script: `InitializeParticle` function call with a `LerpPosition` dynamic input
- `LerpPosition` lerps from A (spawn position) to B (user param target)
- Ribbon renderer traces along particle positions → beam shape

To set up `User.BeamStart` → `User.BeamEnd`:
1. `InitializeParticle` → `Position Mode` pin = `"Direct Set"`
2. `InitializeParticle` → `Position` pin = `User.BeamStart`
3. `LerpPosition` → `B` pin = `User.BeamEnd`

Dump the stage first to get exact names — see `AI/MCP/MCP_Niagara.md`. Steps 2 and 3 link an input to a user
parameter and step 1 sets a static switch, so all three go through the builder utility rather than a route.

## Build.cs

`NiagaraEditor` must stay in `PrivateDependencyModuleNames` unconditionally (not inside `bBuildWithEditorOnlyData`). Moving it to editor-only causes `C1083` on `NiagaraScriptSource.h`.
