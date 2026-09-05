# VFX Knowledge Base

---

## Asset Duplication via Python

The only reliable method — use `load_object` + `AssetTools.duplicate_asset`:
```python
import unreal
obj = unreal.load_object(None, "/Game/Art/VFX/Assets/NS_Square_RurretRecall_Beam.NS_Square_RurretRecall_Beam")
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
result = asset_tools.duplicate_asset("NS_MyCopy", "/Game/Art/VFX/Generic/Niagara", obj)
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
| Turret recall beam | `/Game/Art/VFX/Assets/NS_Square_RurretRecall_Beam` |
| Glow material instance (additive, unlit) | `/Game/Art/VFX/Generic/Materials/MatInstances/MI_Glow01` |
| Unlit particle material | `/Game/Art/VFX/Generic/Materials/M_Particle_Unlit_Advanced` |
| Zone indicator (ring + hard fill) | `/Game/Art/VFX/AOE/M_ZoneIndicator` |
| Zone indicator ray (bar, center→edges) | `/Game/Art/VFX/AOE/M_ZoneIndicatorRay` |
| Pulse circle (outline ring + inward pulsing fill) | `/Game/Art/VFX/AOE/M_PulseCircle` |
| Pulse beam (outline frame + pulse running to target) | `/Game/Art/VFX/AOE/M_PulseBeam` |
| Clock-wipe mask function (remaining-life readout) | `/Game/Art/VFX/Generic/Materials/Functions/MF_DurationWipe` |
| Moira beam niagara (`Beam_Length`/`Beam_Width`/`Color`) | `/Game/Art/VFX/Assets/NS_Cirlce_MoiraBeam` |

Note: The turret recall asset has a typo — "Rurret" not "Turret".

Every project VFX asset lives under `/Game/Art/VFX`, never `/Game/VFX`. Resolve a path against the asset
registry before quoting it — a wrong path fails silently in Python, which loads `None` and carries on.

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

The Go MCP server only forwards JSON fields defined in the tool's schema. Extra fields are silently dropped, so
a route can only ever carry the arguments its schema already names. Anything needing more arguments than that
goes through the editor utility shim from Python instead — never a route operation with the values baked in.
See the generic-functions rule in `AI/MCP/MCP_EditorUtility.md`.

## MCP Niagara Operations (C++)

Operations in `NiagaraRoutes.cpp` (`/api/niagara/ops`): `spawn_system`, `set_parameter`, `get_system_info`,
`add_emitter`, `remove_emitter`, `activate`, `deactivate`. Any other name is rejected as unknown.

### `get_system_info`
Lists emitters and user parameters. Uses `GetUserParameters()` (not `ReadParameterVariables()`) to skip stale redirect aliases.

### `add_emitter` / `remove_emitter`
Both change the handle list without rebuilding the system graph around it — an emitter added this way never
runs and the asset editor cannot open the system. Add emitters through the builder utility instead, which
repairs the whole system, so a removal only needs one add after it. See `AI/MCP/MCP_Niagara.md`.

### Module stacks
These routes do not reach a module stack. Adding modules, setting static switches, writing input values and
nesting dynamic inputs go through the editor utility shim — see `AI/MCP/MCP_Niagara.md`.

## Electric Effects

Systems in `/Game/Art/VFX/Generic/Niagara`. No two share a mechanism, so pick by the read wanted rather than
by tuning one into another. The ring-placed ones carry `User.Radius` so one number fits them to any character;
the mesh-placed ones take their shape from the character's own mesh and ignore it.

| System | Renderer | Placement | Motion | Emission |
|---|---|---|---|---|
| `NS_StaticElectricity` | ribbon | random beam endpoints | curl noise vs drag | restriking bursts |
| `NS_ConeCoil` | ribbon | exec index helix on a cone | vortex velocity | one long strand |
| `NS_SparkFizz` | sprite | random ring surface | outward push vs drag | continuous rate |
| `NS_OrbitArc` | ribbon | orbiting beam endpoints | re-derived per frame | one held strand |
| `NS_ShardStorm` | mesh | random ring surface | inward attraction | restriking bursts |
| `NS_EmpoweredArcRun` | sprite | mesh surface | velocity-aligned drift vs drag | rate plus restriking flash |
| `NS_ChargedTrail` | ribbon | the owner's own path | curl noise vs drag | rate plus restriking bursts |

The beam-placed ones draw their endpoints in emitter scope, so read Random Evaluation Scope before expecting a
strike to move between loops.

`NS_ChargedTrail` is the only one worn by a projectile rather than a character, and the only one whose shape
comes from the owner moving: see Trails Off a Moving Emitter. Its `User.Radius` is the shot's radius, not a
body's. Its character-side counterpart is `NS_ChargedHalo` — see Buff VFX.

Built by `AI/Python/static_electricity_vfx.py`, `cone_coil_vfx.py`, `lightning_variants.py`,
`empowered_arc_vfx.py`, `charged_trail_vfx.py` and `buff_vfx.py`.

## Trails Off a Moving Emitter

A ribbon laid behind a mover needs no event handler and no link-order module. Spawn at a rate, take the
position from the emitter's own, and let the ribbon renderer link particles in birth order: every particle
stays where it was born, so the strand *is* the path travelled. `LocationBasedRibbon` is the template to
duplicate — it carries the ribbon renderer and an `InitializeParticle` already set to offset from the
simulation position, and its own `ReceiveLocationEvent` handler is inert with no source emitter, so adding
`SpawnRate` to Emitter Update is the whole hookup.

This only works because the emitter simulates in world space, which is Niagara's default and cannot be read or
written from Python — `bLocalSpace` lives in the emitter's protected version data. A trail that rides along
with its owner instead of trailing behind it is that checkbox, in the emitter's properties panel.

What makes such a strand read as electricity is the size of the spawn scatter against the gap the owner covers
between two spawns (speed / rate). Scatter each birth on a sphere about as wide as that gap and consecutive
points step sideways as far as they step forward, so the strand is born zigzagged; scatter it smaller and the
strand stays smooth however random the draw. Sample the sphere's surface rather than its volume, or the points
drawn near the centre fall back onto the path and flatten that step.

A curl noise force bends what is already laid, and since a curl field is spatially coherent the strand can only
ever bend as one shape: how many of the field's features span the strand's own length is what separates a lean
from an ondulation. Two layers given the same field at opposite strengths bend into mirror images of each
other, which is how a pair of them braid.

The birth point can only be the owner's own. The initialise-particle module's position-offset input is
addressable and writable but does not move it, so a fixed offset from the owner needs a location module of its
own — and every location module places at random within its shape, which no ribbon can be linked from. Nothing
spreads such a strand while its owner stands still: the rotate-around-a-point modules turn every particle by
the same wall-clock angle, which moves the whole clump without ever separating it, and a clump renders as
nothing at all because a ribbon needs length. A strand that has to hold on a still owner is a beam — see
Strands That Circle an Owner.

## Strands That Circle an Owner

An aura that must read while its owner stands still cannot be a trail, because a trail is the path travelled.
Use a beam instead: two endpoints circling the owner on unrelated periods, with the update-beam module
re-deriving every particle from them each frame, so the chord between them sweeps and stretches and never
settles. `NS_OrbitArc` is the reference for it, and `NS_ChargedHalo` and `NS_VitalHalo` are built on the same
mechanism.

Since the endpoints live in emitter scope, this is also the one placement whose trig actually advances with
time — the same cosine and sine drive nothing in a particle script, where each particle's angle is resolved
once at birth.

## Buff VFX

Everything `UGameDataSettings::BuffVFX` names, one entry per attribute, built by `AI/Python/buff_vfx.py`
except `NS_ChargedTrail`, which is built by `charged_trail_vfx.py`. A character wears the entry's
`CharacterVFX` and the shots it fires wear its `ProjectileVFX`; a shot only carries the two boosts it can
express, so the other three are character-side only.

| System | Attribute | Worn by | The strand is | Emission |
|---|---|---|---|---|
| `NS_ChargedTrail` | DamageMultiplier | the shot | the path travelled | rate plus restriking bursts |
| `NS_VitalTrail` | AppliedHealBoost | the shot | the path travelled | continuous rate |
| `NS_SwiftWake` | MovementSpeedMultiplier | the body | the path travelled | continuous rate |
| `NS_ChargedHalo` | DamageMultiplier | the body | a chord that circles | one held strand plus bursts |
| `NS_VitalHalo` | AppliedHealBoost | the body | three chords that circle | three held strands |
| `NS_MendingDrift` | ReceivedHealBoost | the body | no strand — sprites | continuous rate |
| `NS_BulwarkShell` | DamageReduction | the body | no strand — sprites | continuous rate |

The charged and vital systems are the same mechanisms at opposite settings — scatter, ribbon tension and
colour — so a new attribute is a colour and a tuning, not a new effect. `NS_SwiftWake` is the only one that
deliberately shows nothing at rest: a trail worn by a body draws a contrail while it runs and collapses to a
knot when it stops, which is exactly when a movement-speed buff has nothing to say.

Nothing writes `User.Radius` on these at runtime — a buff system is spawned with no parameters — so each is
authored to the radius of the side it dresses.

## Empowerment Auras

`NS_EmpoweredBlaze`, `NS_EmpoweredSurge` and `NS_EmpoweredArcStorm` are flame auras sampled off the
character's own mesh, built by `AI/Python/empowered_aura_vfx.py`; they differ by what moves the fire — radial
push, a vortex about the up axis, or no push at all. `NS_EmpoweredArcRun` is the electric counterpart and is
built separately.

## Bolt Emitters

A bolt is a beam: `BeamEmitterSetup` names two endpoints, `SpawnBeam` lays a burst of particles between them
in ribbon link order, and the ribbon renderer draws the strand.

- The two endpoints are not symmetric. The start is an absolute position and needs the emitter's own position
  added under it; the end is an offset the module adds that position to itself. Handing the end a position too
  drops the emitter out of it and anchors that half of every strand at the world origin.
- Endpoints are emitter scope, so a burst freezes whatever they held on the frame it fired, and nothing
  re-derives a particle's position afterwards. Giving every strike a new place therefore needs the endpoints
  re-rolled every frame, which is a switch on the random feeding them, not a consequence of restriking — see
  Random Evaluation Scope below. Adding `UpdateBeam` re-derives the whole strand each frame instead and turns
  moving endpoints into a sweeping arc; it has to sit above anything that displaces the strand, or the
  re-derivation wins.
- One burst per loop, with a lifetime shorter than the loop, keeps one strand alive per emitter. Two bursts
  overlapping share the beam's ribbon id and chain into a single snake.

## Making a Strand Read as Lightning

- Split the motion by coherence: a curl noise field bends the whole strand as one shape because neighbouring
  particles are pulled the same way, while jitter is incoherent at every scale and can only ever be the fine
  crackle on top. A strand driven by jitter alone reads as buzzing noise.
- A force against drag settles at a terminal speed, so the bend a bolt picks up over its life is roughly
  strength over drag times lifetime — budget it against the aura's radius rather than tuning the force blind.
- Spawn-time noise only zigzags when its feature size is SMALLER than the gap between two neighbouring points;
  sampled coarser than that, neighbours read almost the same value and the strand merely shifts.
- Ribbon curve tension is sharpness, not smoothing: the higher it is, the sharper the corners. Lightning wants
  it near the maximum. Tessellation cannot be switched off from Python — the mode property is not exposed at
  all, though its factor is.
- The `StaticBeam` emitter template carries a width curve indexed by ribbon link order and a colour curve over
  particle age. Curve keys live in a data interface that no stack edit reaches, so inherit those curves and
  set only their scale.

The base system must loop: a system duplicated from a one-shot completes its emitters on the first tick
whatever their own loop settings say.

`DefaultRibbonMaterial` is additive, unlit and already flagged for ribbons, so an HDR particle colour glows
without authoring a material. Renderer properties are reachable from Python by their exact C++ names
(`Material`, `CurveTension`, `MaxNumRibbons`, `Alignment`, `FacingMode`); the pythonised spelling is refused.
An exposed enum property is writable from Python by assigning the matching `unreal` enum value — sprite
alignment and facing are set that way — but some are not exposed to Python at all, and the Remote Control
property API only reads them. Check a property is readable before assuming it can be written.

## Random Evaluation Scope

A random dynamic input carries an evaluation switch whose entries are spawn-only and every-frame, and
spawn-only resolves the draw once for whatever spawned it. In a particle script that is once per particle,
which is what makes each particle land somewhere of its own. In an emitter script it is once for the emitter's
entire life, so an emitter-scope random is drawn on the first frame and never again, however often the emitter
restrikes underneath it.

That scope difference is the whole reason a restriking bolt can sit at the same two points for a whole run:
the burst repeats, the endpoints do not. Either switch the random to every-frame, or move the placement into a
particle module so every particle draws its own.

The switch is named per script — `Evaluation Type` on the random vector dynamic input, `Random Evaluation` on
the mesh location module — so read the stage dump rather than assuming one name.

## Mesh-Adaptive Placement

`SkeletalMeshLocation` samples the character's own skeletal mesh, so one system fits every character shape and
needs no per-class variant. Its data interface resolves the mesh from its source mode, whose default falls
back to the component the system is attached to when nothing sets a source, so attaching the system to the
character is the whole hookup and no code has to hand it a component. Confirm the enum's meaning in
`NiagaraDataInterfaceSkeletalMesh.h` rather than from the module UI.

Surface sampling covers the whole shape rather than only its rim, which reads as the body itself burning or
conducting rather than as an outline. Sampling modes are chosen by static switch display name, and those names
come from the enum asset, not from the switch's stored entry name.

The asset editor's preview has no attach parent, so a mesh-sampled layer shows nothing there until the
module's preview mesh is set by hand. Judge these systems on a character in a level, never in the preview.

The traverse-skeletal-mesh module family walks a particle across a mesh surface, but its tri-coordinate input
holds a plain value rather than a link and no engine content wires it, so it is unproven here.

## Reading as Electricity Without a Strand

A velocity-aligned sprite sized long on one axis and thin on the other draws a streak lying along its own
direction of travel, so a stream of them reads as current running over a surface without any ribbon or beam.
This is what a mesh-sampled layer can do that a beam cannot, since a beam's endpoints are emitter scope and
cannot come off a particle-scope mesh sample.

Emission style carries meaning on its own: a continuous rate reads as an ambient state, a burst as an event.
An empowerment aura wants the rate; a strike wants the burst, with a spawn probability so it never falls into
an audible beat.

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
