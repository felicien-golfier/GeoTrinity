# AI/Python/Anim

Skeletal animation toolkit and maintenance; the clips live with their category (`Badge/`, `Cat/`, `Boss/`). Rules: `AI/MCP/MCP_Animation.md`. Run `verify_badge_scripts.py` before re-running any badge clip script.

`anim_sequence_authoring.py` is the toolkit every other script imports; it stays at this folder's root, the clip scripts live with their category (`Badge/Anim`, `Cat/Anim`, `Boss/{Hex,Star}/Anim`, `Boss/Anim`) and load it by project-relative path.

## Sparse keys
- Scripts keep sampling per frame (`sampler(frame, bone, rest)`); `write_bone_tracks` turns those samples into keys, per channel (9 per bone), so the clip stays hand-editable in the animation editor.
- Constant channel: no key. Moving channel: keys only where the line bends beyond the tolerance; first and last frame always keyed; linear between keys.
- Tolerances in one block at the top of `anim_sequence_authoring.py` (translation units, rotation degrees, scale); half what `verify_badge_scripts.py` and `sparse_rekey_clips.py` accept.
- Rotation keyed as continuous Euler (unwrapped across frames), so a spin is one run.
- Channels of equal key count sit on the same frames (the model's shared evaluation cache reads by count): the writer pads a colliding channel with keys the line already passes through. Counts are claimed per sequence per write pass.
- Every writer goes through this path (`write_bone_keys`): `write_bone_tracks`, `offset_bone_tracks`, the hex / star clip scripts, `Badge/Anim/rerig_class_badges.py`, `sparse_rekey_clips.py`. A new script never calls the controller's whole-frame setter itself.
- Baked files (`SourceArt/Anim/Baked`) stay full per-frame samples; the writer reduces them on write.

## Toolkit and maintenance
| Script | Does |
|---|---|
| `anim_sequence_authoring.py` | Read a rig, write / offset bone tracks as sparse per-channel keys, build montages, turn a bone about a point, measure overlaps |
| `bake_clips.py` | Freeze clips no script can rebuild into `SourceArt/Anim/Baked` (keys + montage layout) |
| `verify_badge_scripts.py` | Run the badge and boss clip scripts in a sandbox and compare to the live clips frame by frame |
| `sparse_rekey_clips.py` | Rewrite every clip under `Characters/Anim` as sparse keys from what it plays now; saves a clip only when every bone on every frame stays within tolerance |
| `test_sparse_keys.py` | Plain-Python checks of the key reduction (`python AI/Python/Anim/test_sparse_keys.py`, no editor) |
| `export_anim_fbx.py` | Export all badge sequences to `AI/Output/ClassBadgeAnimFbx` per class |
