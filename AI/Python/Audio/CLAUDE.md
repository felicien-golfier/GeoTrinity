# AI/Python/Audio

Run from a shell with `uv run <script>`, outside the editor; each declares its dependencies. Stays one folder (scripts import each other as siblings); boss scores and `gem_kit.py` are not moved to `Boss/` / `Gem/`. Workspace for outputs: `AI/Audio/CLAUDE.md`. Process: `AI/SoundCreation.md`, `AI/MCP/Audacity/`. `Patches/<Pack>/` holds one fitted patch per reproduced sound.

## Analysis
| Script | Does |
|---|---|
| `fetch_reference.py` | Sound-pack video → audio, 720p video, silence cuts, contact sheets, OCR labels |
| `split_on_silence.py` | Cut a montage into one WAV per sound (`sound_spans` used by fetch) |
| `dissect.py` | Envelope, events, partials, noise per octave, modulation, echo, stereo + picture; `survey`, `sound_span` |
| `listen.py` | Perceptual measures + waveform / spectrogram / spectrum PNG |
| `compare_sections.py` | Judge a synthesised section against a reference |
| `match_reference.py` | Fit a model's parameters to a hit in a reference |
| `fit_patch.py` | CMA-ES fit of a patch to its reference; A/B + picture into `AI/Audio/Reproductions` |
| `sfx_patch.py` | Render a JSON patch (oscillators, FM, noise, envelopes, drive, echo, room); free values `{"fit": [v, lo, hi]}` |

## Kits (write to `SourceArt/Audio/<Kit>`)
| Script | Does |
|---|---|
| `heavy_machine.py` | Heavy futuristic machine blocks and one-shots |
| `mech_kit.py` | Mechanical kit (`Mech`): ticks, clicks, impacts, riser, grind, engine, loops |
| `metal_one_shots.py` | Reusable metal one-shots (`Metal`) |
| `ninja_kit.py` | Air and cloth, no metal (`Ninja`) |
| `gem_kit.py` | Crystal chimes for gem drops, no metal (`Gem`): spawn glints, pickup chime with a long ring |
| `zap_kit.py` | Small electric hits for boosted shots (`Elec`) |
| `menu_music_loop.py` | Seamless music loop cut from an mp3 on whole bars, crossfade baked in (`MenuMusic`) |
| `electric_layers.py` | Seamless electric loop, one WAV per layer |
| `hex_lock_layers.py` | Hex boss intro lock clack layers (`hex_lock_params.json`) |

## Boss scores (`ship` writes to `SourceArt`, else draft)
| Script | Does |
|---|---|
| `hex_intro_mech_score.py` | Hex boss intro, mechanical kit: `cues.json`, `loop.json`, preview per hum version |
| `hex_intro_score.py` | Hex boss intro as a heavy machine |
| `hex_boss_montage_score.py` | Hex boss ability + death montages: one-shots on section beats, loop-bending curves |
| `hex_turret_deploy_score.py` | Hex turret deploy montage: hiss, telescoping slide, two-latch lock, notifies on `AM_HexTurret_Deploy` |
| `star_boss_score.py` | Star boss with the ninja kit: fight loop + every montage |

## Compute
- One heavy job at a time (two at most, short ones): `fit_patch.py` CMA-ES fits run ~2.5 cores each, and the editor shares the machine.
- Keep a fit to ~1000–1500 renders and one search unless asked for more; say how many fits and how long before a batch; never leave fits running across turns unannounced.
- Kill by matching the command line, never the MCP servers' Python.
