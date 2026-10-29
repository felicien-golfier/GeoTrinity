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
| `generate_hex_mine_mesh.py` | Hex boss mine: flat hexagon, six spikes alternating long / short (turret cannon shape), core hexagon in its own `Core` slot for the blinking material; every spike its own convex prism |
| `generate_hex_turret_mesh.py` | Hex boss turret (`SM_HexTurret`): the hexagon cut into 4 pieces — two halves of the central rectangle (x within ±R/2, split on y = 0) and two side triangles — plus the hexagonal-section cannon along +X (firing direction) as its own prism |
| `rig_hex_turret.py` | `SM_HexTurret` → `SKM_HexTurret` on `SK_HexTurret`: `Root` (moves in no clip, carries the fire socket), `HalfA` / `HalfB`, `FlapL` / `FlapR` hinged on the rectangle's side edges, `Cannon` |
| `rig_hex_mine.py` | `SM_HexMine` → `SKM_HexMine` on `SK_HexMine`: `Root`, `Core`, `SpikeL1-3` (long), `SpikeS1-3` (short); `Core` keeps its slot |

## Hex/Anim/
| Script | Does |
|---|---|
| `hex_boss_intro.py` `_idle.py` `_death.py` `_abilities.py` `_launch.py` | Hex boss clips: intro, idle, death, beam / ray / cone, tile launches |
| `hex_turret_deploy.py` | `AS_HexTurret_Deploy` + `AM_HexTurret_Deploy`, ~0.8 s: small square block → hexagon + cannon |
| `hex_mine_deploy.py` | `AS_HexMine_Deploy` + `AM_HexMine_Deploy`, ~0.5 s: closed hexagon → six spikes |

## Deploy clips (hex turret, hex mine)
Played as the deployable's `InitMontage` (`Source/GeoTrinity/Public/Actor/Deployable/CLAUDE.md`, "Init phase"): visual only, collision and gameplay are the deployable's, the clip's rest pose is the final look. Authored in the `Cast` slot group like the other boss montages.
- **Turret** (mechanical fold): frame 0 = a small square — `HalfA` over `HalfB` (each ~R × 0.87R), the flaps folded flat beneath, `Cannon` scaled to zero length inside the body. Order: halves swing apart on the Y axis, `FlapL` then `FlapR` hinge out (overshoot, settle), `Cannon` telescopes out last with a small recoil. Seen from above a turning part is foreshortened (`MCP_Animation.md`, "Seen from straight above"): a flap through 180° narrows to a line and widens again; a small `Root` scale-up (~0.7 → 1) on the unfold keeps the pop readable. The fire socket sits on `Root`.
- **Mine** (radial bloom): frame 0 = the closed hexagon, every spike scaled to zero along its length (a zero scale is safe, the width survives for the way back). The three long spikes shoot out first with overshoot, the short ones follow ~3 frames later so the alternation reads; `Core` pulses once as they lock, then rests ready for the fuse blink. `Root` scale ~0.6 → 1.
- Sound (turret only): notifies on the clip, no fight loop. The signature is the turret's own, nothing the boss plays elsewhere — the boss owns clamp slams, cracks, ticks and the engine (`AI/SoundCreation.md`, "Sound spirit per boss"). Turret deploy = pneumatic hiss on the halves, a telescoping slide (quick accelerating ratchet) on the cannon, one two-latch lock at its end; unpitched, never a pitched glide.

Hex ability clips (`hex_boss_abilities.py`): the wind-up is readable from its first frames — a clip's `tell` opens the blade at the boss's front within `tell_frames`, folded on frame 0. Only the blades near the front are out (`tell_focus` above `focus`, so blades 60° off stay almost folded): a turning ring retracts a blade as it leaves the front and opens the next as it arrives. Only the outer ring's blades show — the middle ring's lie inside its shell — and it rests with faces at ±30° of front, not dead on it, so the window must be wide enough to open those. The burst then widens to `focus`.

## Star/Mesh/
| Script | Does |
|---|---|
| `generate_star_mesh.py` | Star boss body: eight points, counter-points, star-shaped heart hole |
| `generate_soft_star_mesh.py` | Friendly dummy body `SKM_SoftStar` (rounded star, pale core slot, on `SK_Star`) + `MI_SoftStar_Body` / `MI_SoftStar_Core`; `BP_Dummy` points at them |
| `rig_star.py` | Rebuild `SKM_Star` on `SK_Star`'s hierarchy, weighted so existing clips play unchanged |

## Star/Anim/
| Script | Does |
|---|---|
| `star_intro.py` `star_idle_breath.py` `star_death.py` | Star boss intro, idle, death |
| `star_devastating_wave.py` `star_pike_nova.py` `star_spike_nova.py` | Star boss attacks |
| `star_intro_camera_shake.py` | Camera-shake curve climbing with the intro montage |
