# AI/Python/Boss

Bosses: hex and star meshes, rigs, clips, fight loop. Rules: `AI/MCP/MCP_Animation.md`; toolkit `Anim/anim_sequence_authoring.py` (loaded by project-relative path). Run `Anim/verify_badge_scripts.py` before re-running any clip script. Scores: `Audio/` (`AI/SoundCreation.md`).

## Shared
| Script | Does |
|---|---|
| `Anim/dump_bone_motion.py` | Sample hex / star boss montage bones to JSON in `AI/Audio/BoneMotion` (sound scores are timed from it) |
| `Anim/boss_montage_sounds.py` | Put each boss montage's score notifies and fight-loop curves on it (`BOSSES` picks which) |
| `boss_fight_loop.py` | Boss fight-loop layers (`loop.json`) onto its loop sound component, keeping tuned drift |

## Hex/Mesh/
| Script | Does |
|---|---|
| `generate_hex_boss_mesh.py` | Hex boss body (three concentric hexagons) + rig data |
| `rig_hex_boss.py` | Hex body → skeletal mesh on a new skeleton |
| `generate_hex_mine_mesh.py` | Hex boss mine: flat hexagon, six spikes alternating long / short (turret cannon shape), core hexagon in its own `Core` slot for the blinking material |
| `generate_hex_turret_mesh.py` | Hex boss turret: flat hexagon + hexagonal-section cannon along +X (firing direction), separate prism for later animation |

## Hex/Anim/
| Script | Does |
|---|---|
| `hex_boss_intro.py` `_idle.py` `_death.py` `_abilities.py` `_launch.py` | Hex boss clips: intro, idle, death, beam / ray / cone, tile launches |

Hex ability clips (`hex_boss_abilities.py`): the wind-up is readable from its first frames — a clip's `tell` opens the blade at the boss's front within `tell_frames`, folded on frame 0. Only the blades near the front are out (`tell_focus` above `focus`, so blades 60° off stay almost folded): a turning ring retracts a blade as it leaves the front and opens the next as it arrives. Only the outer ring's blades show — the middle ring's lie inside its shell — and it rests with faces at ±30° of front, not dead on it, so the window must be wide enough to open those. The burst then widens to `focus`.

## Star/Mesh/
| Script | Does |
|---|---|
| `generate_star_mesh.py` | Star boss body: eight points, counter-points, star-shaped heart hole |
| `rig_star.py` | Rebuild `SKM_Star` on `SK_Star`'s hierarchy, weighted so existing clips play unchanged |

## Star/Anim/
| Script | Does |
|---|---|
| `star_intro.py` `star_idle_breath.py` `star_death.py` | Star boss intro, idle, death |
| `star_devastating_wave.py` `star_pike_nova.py` `star_spike_nova.py` | Star boss attacks |
| `star_intro_camera_shake.py` | Camera-shake curve climbing with the intro montage |
