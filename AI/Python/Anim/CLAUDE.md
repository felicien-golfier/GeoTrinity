# AI/Python/Anim

Skeletal animation and montage authoring. Rules: `AI/MCP/MCP_Animation.md`. Run `verify_badge_scripts.py` before re-running any badge clip script.

`anim_sequence_authoring.py` is the toolkit every other script imports.

## Sparse keys
- Scripts keep sampling per frame (`sampler(frame, bone, rest)`); `write_bone_tracks` turns those samples into keys, per channel (9 per bone), so the clip stays hand-editable in the animation editor.
- Constant channel: no key. Moving channel: keys only where the line bends beyond the tolerance; first and last frame always keyed; linear between keys.
- Tolerances in one block at the top of `anim_sequence_authoring.py` (translation units, rotation degrees, scale); half what `verify_badge_scripts.py` and `sparse_rekey_clips.py` accept.
- Rotation keyed as continuous Euler (unwrapped across frames), so a spin is one run.
- Channels of equal key count sit on the same frames (the model's shared evaluation cache reads by count): the writer pads a colliding channel with keys the line already passes through. Counts are claimed per sequence per write pass.
- Every writer goes through this path (`write_bone_keys`): `write_bone_tracks`, `offset_bone_tracks`, the hex / star clip scripts, `rerig_class_badges.py`, `sparse_rekey_clips.py`. A new script never calls the controller's whole-frame setter itself.
- Baked files (`SourceArt/Anim/Baked`) stay full per-frame samples; the writer reduces them on write.

## Toolkit and maintenance
| Script | Does |
|---|---|
| `anim_sequence_authoring.py` | Read a rig, write / offset bone tracks as sparse per-channel keys, build montages, turn a bone about a point, measure overlaps |
| `bake_clips.py` | Freeze clips no script can rebuild into `SourceArt/Anim/Baked` (keys + montage layout) |
| `verify_badge_scripts.py` | Run the badge and boss clip scripts in a sandbox and compare to the live clips frame by frame |
| `sparse_rekey_clips.py` | Rewrite every clip under `Characters/Anim` as sparse keys from what it plays now; saves a clip only when every bone on every frame stays within tolerance |
| `test_sparse_keys.py` | Plain-Python checks of the key reduction (`python AI/Python/Anim/test_sparse_keys.py`, no editor) |
| `rerig_class_badges.py` | Re-rig badges from `rig_class_badges.py`, rewriting every clip to play as before |
| `export_anim_fbx.py` | Export all badge sequences to `AI/Output/ClassBadgeAnimFbx` per class |
| `dump_bone_motion.py` | Sample hex / star boss montage bones to JSON in `AI/Audio/BoneMotion` (sound scores are timed from it) |
| `boss_montage_sounds.py` | Put each boss montage's score notifies and fight-loop curves on it (`BOSSES` picks which) |

## Class badges
| Script | Does |
|---|---|
| `class_badge_anim_blueprints.py` | One AnimBlueprint per badge: top / bottom slot clips over idle, full-body and additive slots |
| `class_badge_wiring.py` | Put the playable characters on the badges: class data, mesh, montages, materials |
| `class_badge_idle.py` | Breathing, bored part play |
| `class_badge_death.py` | Swell, vanish, shed class debris; sound on first frame |
| `class_badge_rez.py` | 3 s rez: grow back, parts orbit while the body rolls |
| `class_badge_deploy.py` | Throw deployable, full-body slot |
| `class_badge_body_turns.py` | Body-only rolls (Circle through Moira beam, Triangle reeling turrets) |
| `class_badge_circle_charge.py` / `_orbit.py` | Circle charge beam, two variants |
| `class_badge_square_fire.py` / `_piston.py` | Square auto-fire, two variants |
| `class_badge_square_sacrifice.py` | Martyr Beam and Martyr's Wrath, bottom slot |
| `class_badge_triangle_fire.py` | Triangle heavy shot: FireRatchet (auto-fire), FireFormation (turret launch) |
| `class_badge_triangle_reload.py` | Triangle reload, full-body slot |

## Bosses
| Script | Does |
|---|---|
| `hex_boss_intro.py` `_idle.py` `_death.py` `_abilities.py` `_launch.py` | Hex boss clips: intro, idle, death, beam / ray / cone, tile launches |
| `star_intro.py` `star_idle_breath.py` `star_death.py` | Star boss intro, idle, death |
| `star_devastating_wave.py` `star_pike_nova.py` `star_spike_nova.py` | Star boss attacks |
| `star_intro_camera_shake.py` | Camera-shake curve climbing with the intro montage |
