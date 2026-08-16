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

Replacing a vertex's weights drops its other influences, so a re-bound vertex stops following the bones it shared before and moves rigidly with its new one. Parenting a new bone under the one that drove those vertices keeps existing animation on them.

See `AI/Python/anim_sequence_authoring.py` for the bone-adding and vertex-binding helpers.

---

## Writing Bone Tracks

The animation data controller is reached as an editor property on the sequence. Wrap a batch of edits in an open/close bracket pair.

Set the frame rate and frame count before writing keys; the frame count takes a frame-number struct. A sequence holds one more key than its frame count.

A motion with a repeating beat needs a whole number of frames per beat, or the beats fall between samples and come out uneven.

Add a bone's track before setting its keys, unconditionally and ignoring the result — the adder reports failure for an existing track, and the key setter reports success whether or not a track is there. Supply position, rotation and scale arrays of equal length.

Keys land in the sequence's raw model, and the data that actually plays back is built from it separately, so finish a write by finalizing the animation through the animation library.

Confirm the write by evaluating the finished sequence, and confirm the build by reading the sequence's sampled key count against its frame count — pose evaluation reads the raw model and reports the same either way.

Sampling a bone at a frame through the animation library reads that built data instead, which is what the sequence plays.

See `AI/Python/anim_sequence_authoring.py` for the authoring driver.

---

## Making Motion Read as Dynamic

A hit is three beats — a wind-up that accelerates, a few frames at the extreme, a recoil back to rest — and the wind-up takes most of the frames.

Anticipation runs opposite to the action, and the further it goes the larger the action reads.

A vibration on the wind-up alternates every single frame and grows as the wind-up tightens; alternating any slower reads as a wobble.

Hold the last frames of the wind-up dead still — that stillness is what makes the hit land.

Cross from the wind-up's extreme to the action's extreme in two frames, one of them mid-flight, so the spacing itself reads as speed.

Overshoot the extreme by about a tenth on the landing frame, then settle onto it and hold: a pose the eye never rests on does not register.

Never ease the recoil straight into rest — cross rest, swing short of it and settle, the way a spring does.

Drive every part from one curve that each part reads a fixed number of frames late, rather than writing a curve per part: light parts lead, heavy parts drag, and a part still in its wind-up pose while the leading part is out gives the extreme its contrast for nothing.

Under an orthographic camera, motion along the view axis is spent for nothing.

Starting and ending a clip on the reference pose is what lets it blend in and out without a pop.

See `AI/Python/star_spike_nova.py` for a hit built on these, and `AI/Python/star_idle_breath.py` for the looping counterpart.

---

## Re-runnable Asset Scripts

Load an existing asset and rewrite it in place. Deleting a loaded asset routes through the force-delete path, which leaves the package unloadable for the rest of the editor session and the file on disk.

---

## Montage Structure

Creating a montage through its factory with the source-animation property set makes the factory build the slot track and its segment, so a montage that plays a single sequence needs no shim.

Section names and count are readable through the montage's own lookup functions.

The section list and the slot-track array are declared without edit or Blueprint access, and the call that keeps a section's cached segment link consistent is C++ only — writing sections needs a C++ shim, see `MCP_EditorUtility.md`.

A montage whose sections a caller jumps between is only safe to reference once those sections exist; gate the wiring on reading them back.

See `AI/Python/anim_sequence_authoring.py` for the montage builder and the section read-back.

---

## Class Names in Python

A class may be exposed under a script name that differs from its C++ name; search the module's attribute list for the concept before concluding it is unreachable.
