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

Subtracting a bone's local translation from its component translation gives its parent's component position, which reconstructs the hierarchy when no parent accessor is exposed.

A skeleton and the skeletal mesh built on it are separate assets, so anything about deformation needs the mesh rather than the skeleton it is named after.

A skeletal mesh carries its own bone list holding only the bones it is skinned to, so the modifiers report fewer bones than the skeleton asset does.

Which bones actually deform a mesh is readable per vertex from its skin weights, through the modifier object that edits them. A bone that an existing animation moves is not necessarily a bone carrying much of the mesh, so weigh a bone by its vertex count and weight total before building motion on it.

A mesh's bounds give a box and a sphere derived from that box's corner, never the geometry, so they cannot locate a feature. Read vertex positions instead — a static-mesh section yields them directly — and group them by radius and angle to recover the layout that radial motion has to hit.

Nothing exposes a skeletal mesh's geometry to script, only the static-mesh side, so pairing a vertex's weights with where that vertex sits needs an editor shim. The weight modifier indexes the mesh description cloned from the first LOD, so a shim walking that same description lines up index for index — see `MCP_EditorUtility.md`.

A static mesh and the skeletal mesh built from it index their vertices differently, and a mesh's render section, mesh description and weight list each hold a different count, so one is never a stand-in for another.

Evenly spaced radial features keep their phase readable: multiplying every angle by the feature count collapses them onto one direction whose circular mean gives the ring's keying, so no feature has to be assumed to sit at zero degrees.

See `AI/Python/anim_sequence_authoring.py` for the reference-pose, skin-weight, vertex-ring and per-vertex position-with-weights reports.

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

Confirm the write by evaluating the finished sequence.

See `AI/Python/anim_sequence_authoring.py` for the authoring driver.

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
