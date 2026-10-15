# AI/Python/Anim

Skeletal animation and montage authoring. Rules: `AI/MCP/MCP_Animation.md`. Run `verify_badge_scripts.py` before re-running any badge clip script.

`anim_sequence_authoring.py` is the toolkit every other script imports; it stays at this folder's root, the clip scripts live in `Badge/` and `Boss/` and load it by project-relative path.

## Sparse keys
- Scripts keep sampling per frame (`sampler(frame, bone, rest)`); `write_bone_tracks` turns those samples into keys, per channel (9 per bone), so the clip stays hand-editable in the animation editor.
- Constant channel: no key. Moving channel: keys only where the line bends beyond the tolerance; first and last frame always keyed; linear between keys.
- Tolerances in one block at the top of `anim_sequence_authoring.py` (translation units, rotation degrees, scale); half what `verify_badge_scripts.py` and `sparse_rekey_clips.py` accept.
- Rotation keyed as continuous Euler (unwrapped across frames), so a spin is one run.
- Channels of equal key count sit on the same frames (the model's shared evaluation cache reads by count): the writer pads a colliding channel with keys the line already passes through. Counts are claimed per sequence per write pass.
- Every writer goes through this path (`write_bone_keys`): `write_bone_tracks`, `offset_bone_tracks`, the hex / star clip scripts, `Badge/rerig_class_badges.py`, `sparse_rekey_clips.py`. A new script never calls the controller's whole-frame setter itself.
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

## Badge/ — class badges
| Script | Does |
|---|---|
| `rerig_class_badges.py` | Re-rig badges from `Mesh/rig_class_badges.py`, rewriting every clip to play as before |
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

## Boss/ — hex and star bosses
| Script | Does |
|---|---|
| `dump_bone_motion.py` | Sample hex / star boss montage bones to JSON in `AI/Audio/BoneMotion` (sound scores are timed from it) |
| `boss_montage_sounds.py` | Put each boss montage's score notifies and fight-loop curves on it (`BOSSES` picks which) |
| `hex_boss_intro.py` `_idle.py` `_death.py` `_abilities.py` `_launch.py` | Hex boss clips: intro, idle, death, beam / ray / cone, tile launches |
| `star_intro.py` `star_idle_breath.py` `star_death.py` | Star boss intro, idle, death |
| `star_devastating_wave.py` `star_pike_nova.py` `star_spike_nova.py` | Star boss attacks |
| `star_intro_camera_shake.py` | Camera-shake curve climbing with the intro montage |

Hex ability clips (`hex_boss_abilities.py`): the wind-up is readable from its first frames — a clip's `tell` opens the blade at the boss's front within `tell_frames`, folded on frame 0. Only the blades near the front are out (`tell_focus` above `focus`, so blades 60° off stay almost folded): a turning ring retracts a blade as it leaves the front and opens the next as it arrives. Only the outer ring's blades show — the middle ring's lie inside its shell — and it rests with faces at ±30° of front, not dead on it, so the window must be wide enough to open those. The burst then widens to `focus`.
