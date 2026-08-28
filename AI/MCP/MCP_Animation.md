# MCP Animation — Authoring and Reading Animation Assets

Creating and inspecting skeletal animation via `mcp-unreal` `execute_script`.

---

## Reading What a Sequence Animates

Animation lives in a sequencer-backed data model. Read its content by evaluating the sequence into a pose at a series of times and comparing bone transforms against the first sample; the bone-track list, track count, curve-name query, and per-bone pose sampler all read a legacy path that reports empty for every sequence.

Each evaluated pose reuses a shared buffer, so extract the bone transforms you need immediately after evaluating each pose rather than collecting poses into a list.

See `AI/Python/anim_sequence_authoring.py` for the moving-bone report.

---

## Reading the Rig

The reference pose is obtainable from the skeleton alone and yields every bone transform in both local and component space.

A bone's parent is readable by name from the skeleton modifier; deriving it from positions instead cannot separate bones that sit at the same place.

A skeleton and the skeletal mesh built on it are separate assets, so anything about deformation needs the mesh rather than the skeleton it is named after.

A skeletal mesh carries its own bone list holding only the bones it is skinned to, so the modifiers report fewer bones than the skeleton asset does.

Which bones actually deform a mesh is readable per vertex from its skin weights, through the modifier object that edits them. A bone that an existing animation moves is not necessarily a bone carrying much of the mesh, so weigh a bone by its vertex count and weight total before building motion on it.

A mesh's bounds give a box and a sphere derived from that box's corner, never the geometry, so they cannot locate a feature. Read vertex positions instead — a static-mesh section yields them directly — and group them by radius and angle to recover the layout that radial motion has to hit.

Nothing exposes a skeletal mesh's geometry to script, only the static-mesh side, so pairing a vertex's weights with where that vertex sits needs an editor shim. The weight modifier indexes the mesh description cloned from the first LOD, so a shim walking that same description lines up index for index — see `MCP_EditorUtility.md`.

A static mesh and the skeletal mesh built from it index their vertices differently, and a mesh's render section, mesh description and weight list each hold a different count, so one is never a stand-in for another.

Evenly spaced radial features keep their phase readable: multiplying every angle by the feature count collapses them onto one direction whose circular mean gives the ring's keying, so no feature has to be assumed to sit at zero degrees.

Blending a vertex through each of its bones — into that bone's space with its reference component transform, back out with its posed one, weighted and summed — reproduces where the renderer puts it.

A vertex split across bones lands between what those bones do, so a bone track never reads as the silhouette on its own.

A bone carrying only some of a feature's vertices can push that feature past its neighbours but cannot pull it back behind them, because the ones it does not carry hold the silhouette.

Composing a transform in script takes the location first, then the rotation and the scale.

See `AI/Python/anim_sequence_authoring.py` for the reference-pose, hierarchy, skin-weight, vertex-ring, per-vertex position-with-weights and posed-vertex reports.

---

## Editing the Rig

Bones are added through the skeleton modifier, each with a transform in its parent's space, then committed to the mesh. Weights are set per vertex through the weight modifier and committed separately.

Commit the skeleton before touching weights — a vertex cannot be bound to a bone the mesh does not carry yet.

Committing a bone hierarchy the skeleton asset cannot absorb raises a modal merge dialog that stalls an unattended script; adding leaf bones stays on the silent path.

Building a skinned copy of a static mesh over a whole hierarchy at once goes through an editor shim instead: neither factory behind the editor's own conversion command is reachable from script, and the conversion takes the reference skeleton to build against, so nothing afterwards has to reconcile a mesh against a skeleton it was not built on. A reference skeleton wants its root first and every parent declared before its children. Weights are the only half of that reachable from script, so the conversion lands every vertex on the root and a weighting pass moves them onto the bones meant to drive them.

A part that has to disappear gets a bone of its own, placed inside solid geometry, and is scaled away rather than moved; a zero scale is safe because skinning only ever multiplies forward. Scaling its length alone keeps the part's full width and height for when it slides back out.

Replacing a vertex's weights drops its other influences, so a re-bound vertex stops following the bones it shared before and moves rigidly with its new one. Parenting a new bone under the one that drove those vertices keeps existing animation on them.

See `AI/Python/anim_sequence_authoring.py` for the bone-adding and vertex-binding helpers.

---

## Writing Bone Tracks

The animation data controller is reached as an editor property on the sequence. Wrap a batch of edits in an open/close bracket pair.

Set the frame rate and frame count before writing keys; the frame count takes a frame-number struct. A sequence holds one more key than its frame count.

A motion with a repeating beat needs a whole number of frames per beat, or the beats fall between samples and come out uneven.

A stretch of a clip that loops must hold the same pose on its last key as on its first. Where parts read the driving curve at a delay, the plateau preceding that stretch has to be at least as long as the largest delay, or the delayed parts read different values at its two ends.

Add a bone's track before setting its keys, unconditionally and ignoring the result — the adder reports failure for an existing track, and the key setter reports success whether or not a track is there. Supply position, rotation and scale arrays of equal length.

A non-uniform scale written on the root reaches the whole mesh through it: descendants inherit it into their offsets and their own scales alike, so a vertex blended across several bones lands exactly where one scaled transform would put it.

Where that scaling converges is decided by the root's own translation rather than by the scale. Leaving the translation alone converges the shape on the root; scaling it by the same factor converges on the component origin instead, which is the point the shape turns about wherever its rig hangs the root.

Keys land in the sequence's raw model, and the data that actually plays back is built from it separately, so finish a write by finalizing the animation through the animation library.

Confirm the write by evaluating the finished sequence, and confirm the build by reading the sequence's sampled key count against its frame count — pose evaluation reads the raw model and reports the same either way.

Sampling a bone at a frame through the animation library reads that built data instead, which is what the sequence plays.

See `AI/Python/anim_sequence_authoring.py` for the authoring driver.

---

## Making Motion Read as Dynamic

A hit is three beats — a wind-up that accelerates, a few frames at the extreme, a recoil back to rest — and the wind-up takes most of the frames.

Anticipation runs opposite to the action, and the further it goes the larger the action reads.

A vibration on the wind-up alternates every single frame and grows as the wind-up tightens; alternating any slower reads as a wobble.

A vibration on the root shakes the whole shape, while the same vibration on one part reads as that part working against the rest of it.

A vibration inside a stretch that loops has to be counted from that stretch's own first frame, and its period has to divide that stretch's length, or its two ends hold different offsets.

Hold the last frames of the wind-up dead still — that stillness is what makes the hit land, so the vibration stops where it begins rather than running into it.

Cross from the wind-up's extreme to the action's extreme in two frames, one of them mid-flight, so the spacing itself reads as speed.

Overshoot the extreme by about a tenth on the landing frame, then settle onto it and hold: a pose the eye never rests on does not register.

Never ease the recoil straight into rest — cross rest, swing short of it and settle, the way a spring does.

Drive every part from one curve that each part reads a fixed number of frames late, rather than writing a curve per part: light parts lead, heavy parts drag, and a part still in its wind-up pose while the leading part is out gives the extreme its contrast for nothing.

Under an orthographic camera, motion along the view axis is spent for nothing.

Starting and ending a clip on the reference pose is what lets it blend in and out without a pop.

A part with rotational symmetry that turns a whole fraction of a turn matching that symmetry lands on a pose indistinguishable from the one it left, so a rotation can carry across a loop or hold at the end of a clip without ever being unwound backwards.

That same symmetry bounds how fast such a part can be spun: crossing half of it between two frames reads as likely to be turning the other way, and crossing all of it reads as standing still. Stepping one deliberately has the matching bounds — a step of the whole fraction is invisible and half of it reads either way, so a ratchet needs finer teeth than its own symmetry, with a whole number of steps making up one.

Nested parts have to open from the outside in and close from the inside out, whichever order their weights would otherwise give: an inner part swelling into a shell that has not opened yet interpenetrates it. Where a clip both opens and closes, the delay that keeps the parts from moving as one block has to reverse between the two, across a plateau at least as long as the largest delay so that no part jumps as it flips.

A part that slides out of its housing is capped at the reach its mesh gives it: the overshoot that lands a hit belongs to the body, which is meant to swell past where it settles, but past full reach such a part is stretched rather than further out — and a stretched one runs through whatever the part carrying it sits inside.

How far such a part may reach is set by the boundary in the direction it points, so one held still on a corner of that boundary has the most room there is, while one swept across the boundary is held to the shallowest point it crosses.

Weighting a part's extension by how squarely it points where the shape is aimed is bounded by where that part rests: when none of a ring's parts rests aimed that way, a sharp weighting holds every one of them to a fraction of its reach however far it is told to go, and only turning that ring walks one through the aimed direction.

Where several clips are cut from one driver, how each winds up is what tells them apart, so give each its own rather than the one that reads best.

See `AI/Python/star_spike_nova.py` for a hit built on these and `AI/Python/star_idle_breath.py` for the looping counterpart, `AI/Python/hex_boss_idle.py` for a loop closed on symmetry rather than on stillness, and `AI/Python/hex_boss_abilities.py` for several clips driven from one parameter table. The same abilities are cut three ways from three drivers — one eased curve, one run of steps, one spin held at the symmetry's speed limit — in that file and its `_clockwork` and `_frenzy` siblings, and `AI/Python/hex_boss_launch.py` cuts the eased one down to a single beat with no loop and no direction in it.

---

## Making an Actor Read at a Small Size

An actor small on screen reads by its whole silhouette rather than by its parts, so the shape's own scale carries the clip and no amount of motion inside a fixed outline substitutes for it.

Drive that scale from the same curve the parts read and read it undelayed, so it leads every one of them and each part reads a size the shape has already moved.

Draw the whole shape in harder than it goes out, since the clamp is what the action reads against and nothing has to hold it.

A part extended only as far as its mesh gives it disappears at that size, so a legible one is stretched to several times that, and what bounds it is the shell around it rather than taste.

Where a collision volume does not follow the mesh, it caps the size that may be held through a sustained phase, so let the driving curve's overshoot alone drive the extreme and everything below it drive the size held — which puts the size the volume cannot cover on the landing frame and nowhere else.

A sustained phase pulses better shaped like a hit than like a wave — out over a fifth of it and decaying across the rest — since a wave of the same peak spends most of the phase halfway out, which reads as floating.

Check the compounded scale of the shape and its outermost part against that volume on every frame, since either alone stays under a cap the two together pass.

See `AI/Python/hex_boss_abilities.py` for clips cut this way and for the per-frame report of the compounded scale against the cap.

---

## Checking a Clip's Silhouette

A shape's outermost radius cannot show a feature standing proud of a flat face: the shape's corners sit further out than any of its faces, so the feature clears its own surface long before it beats that radius. Project the feature's vertices and its parent's onto the feature's own outward direction and compare the two.

Take that direction from where the bone sits rather than from its axis, which the very scaling being measured would collapse.

Parts nested inside one another have to be checked for interpenetration numerically, since no bone track shows it. The distance from the centre out to a polygonal boundary in a given direction follows from its nearest vertex's radius and direction together, never from that radius alone.

Fit that boundary's centre rather than assuming it sits on the axis, and measure what is inside it from the same fitted centre. A clip that drifts a part off the axis otherwise reads as having shrunk it, and reports an overlap that is not there.

Measure every frame rather than the ones a report prints. Nested parts cross each other over two or three frames, so a sample coarse enough to step past those calls a clip clear that is not.

A clip that only scales needs no mesh to check: the point it converges on is the root's translation with its own scaling undone, and reading that per frame catches a rig whose root sits off the shape.

See `AI/Python/anim_sequence_authoring.py` for the protrusion, boundary-distance and clearance reports.

---

## Re-runnable Asset Scripts

Load an existing asset and rewrite it in place. Deleting a loaded asset routes through the force-delete path, which leaves the package unloadable for the rest of the editor session and the file on disk.

Decide whether the asset is there by attempting that load rather than by asking the asset registry, which reports an asset that is on disk as missing while it is still scanning; creating over it then fails and hands back nothing.

---

## Montage Structure

Creating a montage through its factory with the source-animation property set makes the factory build the slot track and its segment, so a montage that plays a single sequence needs no shim.

Section names and count are readable through the montage's own lookup functions.

A pattern's wind-up section is stretched to the ability's configured delay, so its authored length sets only the proportions inside it.

The jump that takes a montage live matches any section whose name contains the fire section's, so a phase of no fixed length is authored as a second fire section looping on itself until the end jump takes it.

An ability playing a montage of its own rather than a pattern's jumps to the wind-up section on a fresh activation and to a numbered fire section on a repeat, so the section following the wind-up is numbered: an unnumbered one satisfies the test for having a fire section and is then never reached. Nothing jumps to an end section on a fixed-delay ability, so a clip with no phase to hold through needs only those two.

The section list and the slot-track array are declared without edit or Blueprint access, and the call that keeps a section's cached segment link consistent is C++ only — writing sections needs a C++ shim, see `MCP_EditorUtility.md`.

A montage whose sections a caller jumps between is only safe to reference once those sections exist; gate the wiring on reading them back.

See `AI/Python/anim_sequence_authoring.py` for the montage builder and the section read-back.

---

## Notifies

A sequence's notify array is not readable as a property, but the animation library reads and writes both the notifies and the tracks they sit on.

A notify belongs to a named track, so the track has to exist before an event can be added to it.

Clearing every track drops the notifies on them, which is what lets a notify pass be re-run without stacking.

The adder hands back the notify object, and what that notify plays is one of its own properties.

An event's trigger time is read through the library rather than off the event, whose link fields are not exposed.

A notify on a montage links to the segment beneath it, so the slot track has to be in place before the notify is added.

Which animation triggers a given effect is readable from that effect's referencer list, since the notify holding it is a subobject of the animation asset.

See `AI/Python/anim_sequence_authoring.py` for the notify writer and the read-back.

---

## Class Names in Python

A class may be exposed under a script name that differs from its C++ name; search the module's attribute list for the concept before concluding it is unreachable.
