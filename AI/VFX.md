# VFX Knowledge Base

The mechanisms below serve a look, and the look is `AI/ArtDirection.md` — read it before authoring any effect.
Nothing here is a licence to ship a soft one. For editing an emitter's module stack see `AI/MCP/MCP_Niagara.md`.

## Assets

Every project VFX asset lives under `/Game/Art/VFX`, never `/Game/VFX`. Resolve a path against the asset
registry before quoting it — a wrong path fails silently in Python, which loads `None` and carries on.

| Asset | Path |
|---|---|
| Turret recall beam (note the `Rurret` typo) | `/Game/Art/VFX/Assets/NS_Square_RurretRecall_Beam` |
| Moira beam (`Beam_Length`/`Beam_Width`/`Color`) | `/Game/Art/VFX/Assets/NS_Cirlce_MoiraBeam` |
| Glow material instance (additive, unlit) | `/Game/Art/VFX/Generic/Materials/MatInstances/MI_Glow01` |
| Unlit particle material | `/Game/Art/VFX/Generic/Materials/M_Particle_Unlit_Advanced` |
| Clock-wipe mask function | `/Game/Art/VFX/Generic/Materials/Functions/MF_DurationWipe` |
| Zone indicator (ring + hard fill) | `/Game/Art/VFX/AOE/M_ZoneIndicator` |
| Zone indicator ray (bar, center→edges) | `/Game/Art/VFX/AOE/M_ZoneIndicatorRay` |
| Pulse circle (outline ring + inward pulsing fill) | `/Game/Art/VFX/AOE/M_PulseCircle` |
| Pulse beam (outline frame + pulse running to target) | `/Game/Art/VFX/AOE/M_PulseBeam` |

Duplicate a system by loading the object and handing it to the asset tools' duplicate call — the editor asset
library's own duplicate silently returns nothing for a Niagara system, and a raw file copy leaves the package
name as the source's, which the asset registry never recognises. Read an asset entry's path from its package
and asset names; the object-path field is gone in UE5.

A material a mesh renderer draws with has its mesh-particle usage flag set the first time the system renders,
which dirties that material asset. Save it with the system, or the effect falls back to the default material
outside the editor.

## Custom Primitive Data

Slot 0 is the deployable duration wipe and the only slot in use: `AGeoDeployableBase::Tick` writes
`1 - GetDurationPercent()` onto every visual mesh and `M_PulseCircle`'s `DurationSpent` reads it back with
`bUseCustomPrimitiveData`, so one shared material serves every zone instead of a per-actor MID.

The value is the fraction **spent**, never remaining: custom primitive data nobody writes reads 0, so
"remaining" would render every unwritten primitive as fully drained. `PulseCircleInst` on `BP_DamageZone` is
exactly that case — a `GeoEffectZone`, not a deployable, so nothing writes its slot.

Claiming a new slot means picking an unused index and adding it here; the material addresses the slot by index,
so nothing warns when two features collide on one.

`M_PulseBeam` carries the same readout under the same name but as a plain scalar, because it is driven from a
Niagara emitter: a renderer binds material parameters, and custom primitive data belongs to a primitive
component, which a sprite is not.

## Buff VFX

Everything `UGameDataSettings::BuffVFX` names, one entry per attribute. A character wears the entry's
`CharacterVFX` and its shots wear the `ProjectileVFX`; a shot only carries the two boosts it can express.

| System | Attribute | Worn by | Made of | Placement |
|---|---|---|---|---|
| `NS_ChargedTrail` | DamageMultiplier | the shot | ribbon | the path travelled |
| `NS_VitalTrail` | AppliedHealBoost | the shot | ribbon | the path travelled |
| `NS_ChargedHalo` | DamageMultiplier | the body | one hollow triangle, six solid, a snapping three | held frame plus a turning lattice |
| `NS_VitalHalo` | AppliedHealBoost | the body | two dashed rings, hollow motes | two held rings on unrelated periods |
| `NS_BulwarkShell` | DamageReduction | the body | two hexagons, one dashed, solid plates | held shells plus a turning lattice |
| `NS_MendingDrift` | ReceivedHealBoost | the body | hollow and solid triangles | a ring, drawn inward and shrinking |
| `NS_SwiftWake` | MovementSpeedMultiplier | the body | hollow and solid triangles on a strand | dropped on the path and left there |

The two worn by a shot are ribbons because a shot lends its flight path and a path is already a shape. The five
worn by a body are geometry instead: a still body lends nothing, so the shape has to be in the elements and
their placement. `NS_SwiftWake` alone deliberately shows nothing at rest — what it draws is the path, which is
exactly when a movement-speed buff has something to say, and it is the one body-worn system that earns a strand
for the same reason the shot-worn two do.

Nothing writes `User.Radius` on these at runtime — a buff system spawns with no parameters — so each is
authored to the radius of the side it dresses.

Built by `AI/Python/Niagara/buff_vfx.py`, `charged_trail_vfx.py` and `geo_aura_vfx.py`.

## Electric Effects

Systems in `/Game/Art/VFX/Generic/Niagara`. No two share a mechanism, so pick by the read wanted rather than
tuning one into another. The ring-placed ones carry `User.Radius` so one number fits them to any character; the
mesh-placed ones take their shape from the character's mesh and ignore it.

| System | Renderer | Placement | Motion | Emission |
|---|---|---|---|---|
| `NS_StaticElectricity` | ribbon | random beam endpoints | curl noise vs drag | restriking bursts |
| `NS_ConeCoil` | ribbon | exec index helix on a cone | vortex velocity | one long strand |
| `NS_SparkFizz` | sprite | random ring surface | outward push vs drag | continuous rate |
| `NS_OrbitArc` | ribbon | orbiting beam endpoints | re-derived per frame | one held strand |
| `NS_ShardStorm` | mesh | random ring surface | inward attraction | restriking bursts |
| `NS_EmpoweredArcRun` | sprite | mesh surface | velocity-aligned drift vs drag | rate plus restriking flash |
| `NS_ChargedTrail` | ribbon | the owner's own path | curl noise vs drag | rate plus restriking bursts |

The beam-placed ones draw their endpoints in emitter scope — read Random Evaluation Scope before expecting a
strike to move between loops. `NS_ChargedTrail` is the only one worn by a projectile, and the only one whose
shape comes from the owner moving; its `User.Radius` is the shot's radius, and its character-side counterpart
is `NS_ChargedHalo`.

`NS_EmpoweredBlaze`, `NS_EmpoweredSurge` and `NS_EmpoweredArcStorm` are flame auras sampled off the character's
own mesh, differing only in what moves the fire — radial push, a vortex about the up axis, or nothing.
`NS_EmpoweredArcRun` is the electric counterpart.

Built by `AI/Python/Niagara/static_electricity_vfx.py`, `cone_coil_vfx.py`, `lightning_variants.py`,
`empowered_arc_vfx.py`, `empowered_aura_vfx.py` and `charged_trail_vfx.py`.

## Geometric Shapes as Particles

`M_GeoShape` draws a regular polygon per pixel from the sprite's own UVs — no texture, so an edge stays a line
at any size where a shape atlas is only crisp at its authored size. Sides, outline thickness, fill, dashes,
halo and feather are material parameters, so the shape vocabulary is a folder of instances (`MI_GeoShape_*`)
and swapping a shape is swapping the renderer's material. Built by `AI/Python/Material/geo_shape_material.py`, whose
header carries the math.

- The apothem is derived from the side count rather than authored, inscribing every polygon in one circle.
  Without it a triangle's corners reach twice as far as its edges, so one sprite size would mean a different
  apparent size per shape and a triangle would be clipped by its own quad.
- The halo is centred on the shape's edge, never the middle: a falloff peaked at the centre fills a hollow
  ring's disc with the blob the shape exists to avoid. It also needs a radial term, since the polygon falloff
  does not reach zero on a square quad's boundary and the sprite's corners show through.
- Thickness and feather are fractions of the shape, so a big frame and a small shard cannot share an instance —
  the frame wants a thin precise line, and a feather is only crisp while it covers about a pixel.
- One instance per shape *and fill*: hollow and solid in one effect are two emitters, fixing the ratio by
  authoring rather than by chance. The material also takes fill from dynamic parameter 1 per particle.
- Set a renderer's material on the layer source emitter, before it is copied into a system. Inside a system an
  emitter's object name is not its handle name, so a renderer resolved by name there can belong to a different
  copy: the values land on the right emitter and the material on another, which draws the template's own soft
  sprite and reads as a blurred ball.

## Rings Without Randomness

`ShapeLocation`'s Ring / Disc distribution takes `Direct`, `Uniform` or `Random`. `Uniform` spaces a burst at
exactly 360/N from the execution indices with nothing to nest under it — that lattice, not a scatter, is what
makes an aura read as a machine. It is its own branch and exposes neither `Disc Coverage` nor `U Distribution`,
which belong to the random one.

## Auras Ride Their Owner

An effect worn by a body has to simulate in local space or its particles stay where the body was, leaving the
aura standing in the arena. Local space lives in the emitter's versioned data, which Python refuses as
deprecated for reading as much as for writing and whose owning property is private, so the editor shim reaches
it by reflection instead — and with it everything else that struct holds, persistent IDs and determinism and
fixed bounds. Write it on the layer's own emitter asset before that emitter is copied into a system, the same
constraint the renderer's material carries and for the same reason.

Nothing else fixes it: a vortex or a rotate-around-point re-derives a particle from wherever the owner now is,
without ever closing the distance already lost.

## What an Aura Sheds

One layer has to do the opposite, and it is what gives the aura a trail: small, fast, simulating in world space,
so its shapes are left where they were born and the body walks out of them. A running body drags a stream
behind it, a still one gets a pulse in place. Both halves are needed — everything held rides along and cannot
show speed, and a shed layer alone has no aura to belong to. A shed layer is an ordinary sprite emitter with
local space cleared, so it needs no template of its own.

A module working from a point — an attraction, a velocity out of a point — reads that point off the owner, so a
world-space layer follows a moving body with nothing wired: its pull resolves to wherever the body now is, which
streams shapes after a runner and gathers them straight in on someone standing. A slow drift out of the body
keeps a shed layer moving while its owner is not, and is also the only thing a strand could be drawn along.

A shed layer can carry a strand as well: a ribbon renderer beside its sprites, threading the shapes it already
leaves behind. The renderer list shares the versioned data that refuses local space and no ribbon class is
exposed to Python, so the shim creates the renderer from a class path; every property on it is plain Python
afterwards, since Python wraps the object as the base renderer class and still resolves properties against the
real one.

A ribbon chains several particles, so no single shape ever trails itself — a per-shape strand needs each shape
emitting its own particles, which is a different machine. What one strand does draw is birth order, which for a
layer born at the body is the path travelled. Give it to a layer spawned on a ring and consecutive shapes sit at
unrelated angles, so the strand zigzags between them instead of following anything.

## Trails Off a Moving Emitter

A ribbon laid behind a mover needs no event handler and no link-order module: spawn at a rate, take the position
from the emitter's own, and let the ribbon renderer link particles in birth order, so every particle stays where
it was born and the strand *is* the path travelled. `LocationBasedRibbon` is the template to duplicate — it
carries the ribbon renderer and an `InitializeParticle` already offsetting from the simulation position, and its
`ReceiveLocationEvent` handler is inert with no source emitter, so adding `SpawnRate` to Emitter Update is the
whole hookup. It works only because the emitter simulates in world space.

What makes such a strand read as electricity is the size of the spawn scatter against the gap the owner covers
between two spawns (speed / rate). Scatter each birth on a sphere about as wide as that gap and consecutive
points step sideways as far as they step forward, so the strand is born zigzagged; smaller and it stays smooth
however random the draw. Sample the sphere's surface, not its volume, or points drawn near the centre fall back
onto the path and flatten the step.

A curl noise force bends what is already laid, and since a curl field is spatially coherent the strand can only
bend as one shape: how many of the field's features span the strand's length separates a lean from an
ondulation. Two layers given the same field at opposite strengths bend into mirror images, which is how a pair
of them braid.

The birth point can only be the owner's own. The initialise-particle module's position-offset input is
addressable and writable but does not move it, so a fixed offset needs a location module of its own — and every
location module places at random within its shape, which no ribbon can be linked from. Nothing spreads such a
strand while its owner stands still: the rotate-around-a-point modules turn every particle by the same
wall-clock angle, moving the whole clump without separating it, and a clump renders as nothing because a ribbon
needs length. A strand that has to hold on a still owner is a beam.

## Strands That Circle an Owner

An aura that must read while its owner stands still cannot be a trail, because a trail is the path travelled.
Use a beam: two endpoints circling the owner on unrelated periods with the update-beam module re-deriving every
particle from them each frame, so the chord between them sweeps and stretches and never settles. `NS_OrbitArc`
is the reference; `NS_ChargedHalo` and `NS_VitalHalo` are built on it.

Since the endpoints live in emitter scope, this is also the one placement whose trig actually advances with
time — the same cosine and sine drive nothing in a particle script, where each particle's angle resolves once
at birth.

## Bolt Emitters

A bolt is a beam: `BeamEmitterSetup` names two endpoints, `SpawnBeam` lays a burst of particles between them in
ribbon link order, and the ribbon renderer draws the strand.

- The endpoints are not symmetric. The start is an absolute position and needs the emitter's own position added
  under it; the end is an offset the module adds that position to itself. Handing the end a position too drops
  the emitter out of it and anchors that half of every strand at the world origin.
- Endpoints are emitter scope, so a burst freezes whatever they held on the frame it fired and nothing
  re-derives a particle afterwards. Giving every strike a new place needs the endpoints re-rolled every frame,
  which is a switch on the random feeding them, not a consequence of restriking. Adding `UpdateBeam` re-derives
  the whole strand each frame instead and turns moving endpoints into a sweeping arc; it has to sit above
  anything displacing the strand, or the re-derivation wins.
- One burst per loop, with a lifetime shorter than the loop, keeps one strand alive per emitter. Two overlapping
  bursts share the beam's ribbon id and chain into a single snake.
- The base system must loop: a system duplicated from a one-shot completes its emitters on the first tick
  whatever their own loop settings say.

### Making a strand read as lightning

- Split the motion by coherence: a curl noise field bends the whole strand as one shape because neighbouring
  particles are pulled the same way, while jitter is incoherent at every scale and can only be the fine crackle
  on top. A strand driven by jitter alone reads as buzzing noise.
- A force against drag settles at a terminal speed, so the bend a bolt picks up is roughly strength over drag
  times lifetime — budget it against the aura's radius rather than tuning blind.
- Spawn-time noise only zigzags when its feature size is smaller than the gap between neighbouring points;
  coarser than that, neighbours read almost the same value and the strand merely shifts.
- Ribbon curve tension is sharpness, not smoothing — the higher it is, the sharper the corners, and lightning
  wants it near maximum. Tessellation cannot be switched off from Python: the mode property is not exposed,
  though its factor is.
- The `StaticBeam` template carries a width curve indexed by ribbon link order and a colour curve over particle
  age. Curve keys live in a data interface no stack edit reaches, so inherit those curves and set only their
  scale.

`DefaultRibbonMaterial` is additive, unlit and already flagged for ribbons, so an HDR particle colour glows with
no material authored. Renderer properties are reachable from Python by their exact C++ names (`Material`,
`CurveTension`, `MaxNumRibbons`, `Alignment`, `FacingMode`) — the pythonised spelling is refused. An exposed
enum property is writable by assigning the matching `unreal` enum value, but some are not exposed to Python at
all and the property API only reads them, so check a property is readable before assuming it can be written.

## Random Evaluation Scope

A random dynamic input carries an evaluation switch whose entries are spawn-only and every-frame, and
spawn-only resolves the draw once for whatever spawned it. In a particle script that is once per particle,
which is what makes each particle land somewhere of its own; in an emitter script it is once for the emitter's
entire life, so an emitter-scope random is drawn on the first frame and never again however often the emitter
restrikes underneath it. That is the whole reason a restriking bolt can sit at the same two points for a run.
Either switch the random to every-frame, or move the placement into a particle module.

The switch is named per script — `Evaluation Type` on the random vector dynamic input, `Random Evaluation` on
the mesh location module — so read the stage dump rather than assuming one name.

## Mesh-Adaptive Placement

`SkeletalMeshLocation` samples the character's own skeletal mesh, so one system fits every character and needs
no per-class variant. Its data interface resolves the mesh from its source mode, whose default falls back to
the component the system is attached to, so attaching the system to the character is the whole hookup and no
code hands it a component. Confirm the enum's meaning in `NiagaraDataInterfaceSkeletalMesh.h`, not from the
module UI.

Surface sampling covers the whole shape rather than only its rim, which reads as the body itself burning or
conducting rather than as an outline. Sampling modes are chosen by static switch display name, and those names
come from the enum asset, not from the switch's stored entry name.

The asset editor's preview has no attach parent, so a mesh-sampled layer shows nothing there until the module's
preview mesh is set by hand — judge these on a character in a level. The traverse-skeletal-mesh module family
walks a particle across a surface, but its tri-coordinate input holds a plain value rather than a link and no
engine content wires it, so it is unproven here.

## Reading as Electricity Without a Strand

A velocity-aligned sprite sized long on one axis and thin on the other draws a streak lying along its direction
of travel, so a stream of them reads as current running over a surface with no ribbon or beam. This is what a
mesh-sampled layer can do that a beam cannot, since a beam's endpoints are emitter scope and cannot come off a
particle-scope mesh sample.

Emission style carries meaning on its own: a continuous rate reads as an ambient state, a burst as an event. An
empowerment aura wants the rate; a strike wants the burst, with a spawn probability so it never falls into an
audible beat.

## Niagara Parameter Store

Every `User.*` name the game code writes is declared once in
`Source/GeoTrinity/Public/Tool/GeoNiagaraParams.h` — never spell one at a call site. Adding or renaming a user
parameter on a system means changing it there too, and nothing else will tell you: an unmatched name is silently
ignored and the system keeps its authored default.

Renaming leaves a stale redirect alias in `UserParameterRedirects`, so read user parameters through
`GetUserParameters()`, which skips aliases. `RecreateRedirections()` is not exported from the Niagara DLL and
cannot be called.

## Beam Wiring by User Parameter

The turret recall beam spawns with an `InitializeParticle` call carrying a `LerpPosition` dynamic input that
lerps from the spawn position to a user parameter, and the ribbon renderer traces the particle positions. To
drive it from two user parameters: set `Position Mode` to direct-set, link `Position` to the start parameter and
`LerpPosition`'s far end to the end parameter. Dump the stage first for exact names; the two links and the
switch all go through the builder utility rather than a route — see `AI/MCP/MCP_Niagara.md`.
