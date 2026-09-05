# MCP Animation — Authoring and Reading Animation Assets

Creating and inspecting skeletal animation through `execute_script`. Every report and writer named below lives
in `AI/Python/Anim/anim_sequence_authoring.py` unless another file is given.

## Reading a sequence

Animation lives in a sequencer-backed data model. Read its content by evaluating the sequence into a pose at a
series of times and comparing bone transforms against the first sample — the bone-track list, track count,
curve-name query and per-bone pose sampler all read a legacy path that reports empty for every sequence. Each
evaluated pose reuses a shared buffer, so extract the transforms you need immediately after each evaluation
rather than collecting poses into a list.

## Reading the rig

- The reference pose comes from the skeleton alone, in both local and component space. A bone's parent is
  readable by name from the skeleton modifier; deriving it from positions cannot separate bones sitting at the
  same place.
- A skeleton and the skeletal mesh built on it are separate assets, so anything about deformation needs the
  mesh. The mesh carries only the bones it is skinned to, so its modifiers report fewer bones than the skeleton.
- Which bones deform a mesh is readable per vertex from its skin weights. A bone an existing animation moves is
  not necessarily one carrying much of the mesh, so weigh a bone by its vertex count and weight total before
  building motion on it.
- Bounds give a box and a sphere derived from that box's corner, never the geometry, so they cannot locate a
  feature. Read vertex positions instead — a static-mesh section yields them directly — and group them by radius
  and angle to recover the layout radial motion has to hit. Evenly spaced radial features keep their phase
  readable: multiplying every angle by the feature count collapses them onto one direction whose circular mean
  gives the ring's keying, so no feature has to be assumed to sit at zero degrees.
- Nothing exposes a skeletal mesh's geometry to script, only the static-mesh side, so pairing a vertex's weights
  with where it sits needs a shim. The weight modifier indexes the mesh description cloned from the first LOD,
  so a shim walking that same description lines up index for index. A static mesh and the skeletal mesh built
  from it index vertices differently, and a mesh's render section, mesh description and weight list each hold a
  different count — one is never a stand-in for another.
- Blending a vertex through each of its bones — into that bone's space with its reference component transform,
  back out with its posed one, weighted and summed — reproduces where the renderer puts it. A vertex split
  across bones lands between what those bones do, so a bone track never reads as the silhouette on its own, and
  how far between them it lands is that bone's weight: a scale written on a ring bone reaches the silhouette
  only in that share. Two rings given different scales can therefore cancel into a plain uniform shrink, which
  reads as the shape seen smaller rather than as a change of shape. A bone carrying only some of a feature's
  vertices can push that feature past its neighbours but never pull it back behind them, because the ones it
  does not carry hold the silhouette.
- Composing a transform in script takes the location first, then the rotation and the scale.

## Editing the rig

Bones are added through the skeleton modifier, each with a transform in its parent's space, then committed to
the mesh; weights are set per vertex through the weight modifier and committed separately. Commit the skeleton
first — a vertex cannot be bound to a bone the mesh does not carry yet. Committing a hierarchy the skeleton
asset cannot absorb raises a modal merge dialog that stalls an unattended script; adding leaf bones stays on the
silent path.

Building a skinned copy of a static mesh over a whole hierarchy goes through a shim: neither factory behind the
editor's own conversion command is reachable from script, and the conversion takes the reference skeleton to
build against, so nothing afterwards has to reconcile a mesh against a skeleton it was not built on. A reference
skeleton wants its root first and every parent declared before its children. Weights are the only half reachable
from script, so the conversion lands every vertex on the root and a weighting pass moves them onto the bones
meant to drive them.

Replacing a vertex's weights drops its other influences, so a re-bound vertex stops following the bones it
shared before and moves rigidly with its new one; parenting a new bone under the one that drove those vertices
keeps existing animation on them. A part that has to disappear gets a bone of its own, placed inside solid
geometry, and is scaled away rather than moved — a zero scale is safe because skinning only multiplies forward,
and scaling its length alone keeps the part's full width and height for when it slides back out.

## Writing bone tracks

The animation data controller is an editor property on the sequence; wrap a batch of edits in an open/close
bracket pair. Set the frame rate and frame count before writing keys (the count takes a frame-number struct); a
sequence holds one more key than its frame count, and a motion with a repeating beat needs a whole number of
frames per beat or the beats fall between samples.

Add a bone's track before setting its keys, unconditionally and ignoring the result — the adder reports failure
for an existing track, and the key setter reports success whether or not a track is there. Supply position,
rotation and scale arrays of equal length.

Keys land in the sequence's raw model and the data that actually plays back is built from it separately, so
finish a write by finalizing through the animation library. Confirm the write by evaluating the sequence and the
build by reading its sampled key count against its frame count — pose evaluation reads the raw model and reports
the same either way, while sampling a bone at a frame through the animation library reads the built data.

A non-uniform scale on the root reaches the whole mesh: descendants inherit it into their offsets and their own
scales alike, so a vertex blended across several bones lands exactly where one scaled transform would put it.
Where that scaling converges is decided by the root's translation, not the scale — leaving the translation alone
converges the shape on the root, scaling it by the same factor converges on the component origin, which is the
point the shape turns about wherever its rig hangs the root.

A stretch that loops must hold the same pose on its last key as on its first. Where parts read the driving curve
at a delay, the plateau preceding that stretch has to be at least as long as the largest delay, or the delayed
parts read different values at its two ends.

## Making motion read as dynamic

- **A hit is three beats** — a wind-up that accelerates, a few frames at the extreme, a recoil back to rest —
  and the wind-up takes most of the frames. Anticipation runs opposite to the action, and the further it goes
  the larger the action reads.
- **Vibration** on the wind-up alternates every single frame and grows as the wind-up tightens; slower reads as
  a wobble. On the root it shakes the whole shape, on one part it reads as that part working against the rest.
  Inside a looping stretch it is counted from that stretch's own first frame and its period has to divide that
  length, or the two ends hold different offsets.
- **Hold the last wind-up frames dead still** — that stillness is what makes the hit land, so the vibration
  stops where it begins. A wind-up that accelerates a turn stops that turn there too: a spin carried through the
  stillness spends it, and stopping one at its fastest is the strongest reading a wind-up has.
- **Cross to the action's extreme in two frames**, one of them mid-flight, so the spacing itself reads as speed.
  Overshoot by about a tenth on the landing frame, then settle and hold — a pose the eye never rests on does not
  register. Never ease the recoil straight into rest: cross rest, swing short of it and settle, like a spring.
- **Drive every part from one curve** that each reads a fixed number of frames late rather than writing a curve
  per part: light parts lead, heavy parts drag, and a part still in its wind-up pose while the leading part is
  out gives the extreme its contrast for nothing.
- Under an orthographic camera, motion along the view axis is spent for nothing. Starting and ending a clip on
  the reference pose is what lets it blend in and out without a pop.
- **Rotational symmetry**: a part that turns a whole fraction of a turn matching its own symmetry lands on an
  indistinguishable pose, so a rotation can carry across a loop or hold at a clip's end without being unwound.
  Writing such a turn as a rate integrated across the clip and normalised to its own total, rather than as an
  angle, lets it accelerate, freeze and bleed off however the shot wants and still land on a whole fraction; a
  rate taken from how much of the shape has arrived escalates on its own. That symmetry also bounds the speed:
  crossing half of it between two frames reads as turning the other way and crossing all of it reads as standing
  still, so a deliberate ratchet needs finer teeth than its own symmetry, with a whole number of steps making up
  one.
- **Nested parts** open from the outside in and close from the inside out, whichever order their weights would
  give — an inner part swelling into a shell that has not opened yet interpenetrates it. Where a clip both opens
  and closes, the delay keeping the parts from moving as one block reverses between the two, across a plateau at
  least as long as the largest delay so no part jumps as it flips.
- **A part that slides out of its housing** is capped at the reach its mesh gives it: the overshoot that lands a
  hit belongs to the body, which is meant to swell past where it settles, while past full reach such a part is
  stretched rather than further out — and a stretched one runs through whatever it sits inside. A bone carrying
  only the far end of a feature can push it out but never draw it in, since pulled the other way it turns the
  feature inside out; what draws such features back is the scale of the ring they hang off. How far one may
  reach is set by the boundary in the direction it points, so one held still on a corner of that boundary has
  the most room there is, while one swept across the boundary is held to the shallowest point it crosses.
  Weighting a part's extension by how squarely it points where the shape is aimed is bounded by where it rests:
  when none of a ring's parts rests aimed that way, a sharp weighting holds every one to a fraction of its reach
  however far it is told to go, and only turning that ring walks one through the aimed direction.
- Where several clips are cut from one driver, how each winds up is what tells them apart, so give each its own
  rather than the one that reads best.

`AI/Python/Anim/star_spike_nova.py` is a hit built on these and `star_idle_breath.py` the looping counterpart;
`hex_boss_idle.py` closes a loop on symmetry rather than stillness, and `hex_boss_abilities.py` drives several
clips from one parameter table — the same abilities are cut three ways from three drivers (one eased curve, one
run of steps, one spin held at the symmetry's speed limit) across that file and its `_clockwork` and `_frenzy`
siblings, while `hex_boss_launch.py` cuts the eased one to a single beat with no loop and no direction in it.

## Bringing a shape together

A clip that assembles a shape runs on two clocks: until every part is home each keeps its own schedule, since
arriving one at a time is the whole point of that half, and from there one curve drives everything with the
usual per-part delays. Such a clip does not start on the reference pose, so its montage blends in over nothing —
a blend drags the scattered parts out of whatever pose the shape was holding.

A part waiting its turn has to clear whatever is already home for the whole wait, not only as it lands: parked
closer than the assembled shape's radius plus its own, it sits inside that shape for every frame of the wait,
which reads as a mistake rather than as a part that has not arrived. The only frames a pair may overlap are the
crossing's, so a schedule is checked by counting the frames a pair touches before the later one lands rather
than by how deep the touch goes. Nesting clearance cannot answer that, since a part not yet home is outside the
shell by definition; what answers it is the separation of whole parts, each centre measured against its own
outermost reach.

Arrivals whose gaps shrink by a fixed ratio escalate on their own, so the rhythm needs no beat-by-beat tuning
and follows however many parts the rig carries. Where the parts are a ring of like features, waking them on a
stride coprime with their count visits every one exactly once and reads as the shape waking all over, where
stepping to the neighbour reads as a sweep round it. Every ordering fine enough to state as a rule is regular
enough to be seen as one, so a ring that has to read as firing wrongly rather than in turn is shuffled from a
fixed seed. A feature thrown out one at a time has to stay out once it is, or the ring reads as a wave
travelling round it rather than as features that have gone. A shape with no separable parts assembles the same
way through its own features: what arrives one at a time is a feature thrown out of a body that swells to meet
it, and the shape gains one live part per beat as surely as a scattered one does.

Taking one apart is not that clip reversed. A shape coming together is winding up to be whole, so it may hold
still before the beat that finishes it; one breaking has nothing to wind up for, and a frame of stillness
anywhere in it reads as intent. Where such a clip ends on its parts at rest rather than cleared away, each has
to stop turning on the frame it stops moving, or the wreckage keeps spinning where it lies.

See `AI/Python/Anim/hex_boss_intro.py` (parts that exist from the first frame and fly together), `star_intro.py` (a
shape with nothing to scatter, growing its own points), `hex_boss_death.py` (parts leaving the axis one at a
time and thrown apart to stay) and `star_death.py` (a shape that throws its features out and swallows them).

## Making an actor read at a small size

An actor small on screen reads by its whole silhouette rather than by its parts, so the shape's own scale
carries the clip and no amount of motion inside a fixed outline substitutes for it. Drive that scale from the
same curve the parts read and read it undelayed, so it leads every one of them and each part reads a size the
shape has already moved. Draw the whole shape in harder than it goes out, since the clamp is what the action
reads against and nothing has to hold it.

A part extended only as far as its mesh gives it disappears at that size, so a legible one is stretched to
several times that, bounded by the shell around it rather than by taste. Where a collision volume does not
follow the mesh it caps the size that may be held through a sustained phase, so let the driving curve's
overshoot alone drive the extreme and everything below it drive the size held — which puts the size the volume
cannot cover on the landing frame and nowhere else. Check the compounded scale of the shape and its outermost
part against that volume on every frame, since either alone stays under a cap the two together pass.

A sustained phase pulses better shaped like a hit than like a wave — out over a fifth of it and decaying across
the rest — since a wave of the same peak spends most of the phase halfway out, which reads as floating.

`AI/Python/Anim/hex_boss_abilities.py` carries clips cut this way and the per-frame report of compounded scale
against the cap.

## Checking a clip's silhouette

- A shape's outermost radius cannot show a feature standing proud of a flat face: the corners sit further out
  than any face, so the feature clears its own surface long before it beats that radius. Project the feature's
  vertices and its parent's onto the feature's own outward direction and compare. Take that direction from where
  the bone sits, not from its axis, which the very scaling being measured would collapse.
- Parts nested inside one another have to be checked for interpenetration numerically, since no bone track shows
  it. The distance from the centre out to a polygonal boundary in a given direction follows from its nearest
  vertex's radius and direction together, never from that radius alone. Fit that boundary's centre rather than
  assuming it sits on the axis, and measure what is inside it from the same fitted centre — a clip that drifts a
  part off the axis otherwise reads as having shrunk it and reports an overlap that is not there.
- Measure every frame, not the ones a report prints: nested parts cross over two or three frames, so a coarser
  sample calls a clip clear that is not.
- A clip that only scales needs no mesh to check — the point it converges on is the root's translation with its
  own scaling undone, and reading that per frame catches a rig whose root sits off the shape.
- A silhouette measured off the rig is in the mesh's own units, which the actor's scale sits between and the
  camera, so it answers only as a ratio against the same clip's rest pose and never as a fraction of the view's
  width. That ratio is what a reach is authored in: the same number of units is a fraction of one rig's rest
  radius and several times another's.
- A uniform scale on the root cannot bring two parts together, so an overshoot placed there costs no clearance
  and the per-part scales stay at the values that were checked.
- A clip that folds features away does not end on the reference pose, so comparing the whole skeleton against
  that pose reports the fold as a mismatch; compare the parts that must return and check the folded ones by
  protrusion.

## Montages

Creating a montage through its factory with the source-animation property set makes the factory build the slot
track and its segment, so a montage playing a single sequence needs no shim. Section names and count are
readable through the montage's own lookup functions, but the section list and slot-track array are declared
without edit or Blueprint access and the call keeping a section's cached segment link consistent is C++ only, so
writing sections needs a shim.

A pattern's wind-up section is stretched to the ability's configured delay, so its authored length sets only the
proportions inside it. The jump that takes a montage live matches any section whose name contains the fire
section's, so a phase of no fixed length is authored as a second fire section looping on itself until the end
jump takes it. An ability playing a montage of its own rather than a pattern's jumps to the wind-up section on a
fresh activation and to a numbered fire section on a repeat, so the section following the wind-up is numbered —
an unnumbered one satisfies the test for having a fire section and is then never reached. Nothing jumps to an
end section on a fixed-delay ability, so a clip with no phase to hold through needs only those two.

A montage whose sections a caller jumps between is only safe to reference once those sections exist; gate the
wiring on reading them back.

## Notifies

A sequence's notify array is not readable as a property, but the animation library reads and writes both the
notifies and the tracks they sit on. A notify belongs to a named track, so the track has to exist before an
event is added to it, and clearing every track drops the notifies on them — which is what lets a notify pass be
re-run without stacking. The adder hands back the notify object, and what it plays is one of its own properties;
an event's trigger time is read through the library rather than off the event, whose link fields are not
exposed. A notify on a montage links to the segment beneath it, so the slot track has to be in place first.
Which animation triggers a given effect is readable from that effect's referencer list, since the notify holding
it is a subobject of the animation asset.

## Script notes

Load an existing asset and rewrite it in place. Deleting a loaded asset routes through the force-delete path,
which leaves the package unloadable for the rest of the session and the file on disk. Decide whether the asset
is there by attempting that load rather than by asking the asset registry, which reports an on-disk asset as
missing while it is still scanning — creating over it then fails and hands back nothing.

A class may be exposed under a script name differing from its C++ name; search the module's attribute list for
the concept before concluding it is unreachable.
