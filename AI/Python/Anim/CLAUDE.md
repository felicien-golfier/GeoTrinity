# AI/Python/Anim

Skeletal animation and montage authoring. Rules: `AI/MCP/MCP_Animation.md`. Run `verify_badge_scripts.py` before re-running any badge clip script.

`anim_sequence_authoring.py` is the toolkit every other script imports.

## Toolkit and maintenance
| Script | Does |
|---|---|
| `anim_sequence_authoring.py` | Read a rig, write / offset bone tracks, build montages, turn a bone about a point, measure overlaps |
| `bake_clips.py` | Freeze clips no script can rebuild into `SourceArt/Anim/Baked` (keys + montage layout) |
| `verify_badge_scripts.py` | Run every badge clip script in a sandbox and compare to the live clips frame by frame |
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
