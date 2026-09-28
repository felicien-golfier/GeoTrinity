# Sound Creation

The route for a game sound, from an intent or a reference clip to a Sound Wave playing on its beat. The craft —
layers, sound grammar, recipes — is `AI/MCP/Audacity/GameSoundDesign.md`; judging a sound by numbers and matching
a reference is `AI/MCP/Audacity/ListeningToSound.md`; driving Audacity is `AI/MCP/Audacity/CLAUDE.md`.

## Choosing the route

| The sound | Route |
|---|---|
| A simple tonal cue — a blip, a chirp, a sweep, a few layers | Audacity's generators, one track per layer |
| Noise textures, click trains, loops, per-band decays, stereo width | A Python synth in `AI/Python/Audio/`, one WAV per layer, the layers loaded into Audacity |
| "Make it sound like this" — a reference clip | `AI/Python/Audio/match_reference.py`: measure the reference, fit a Python synth, compare |

- Audacity generates only tones, chirps, plain noise and an even click track; anything irregular, gliding or
  band-dependent is synthesised in Python.
- The agent judges by measurement, and a fit takes thousands of renders: that loop only runs in Python.
- Audacity stays the desk the user listens and balances at: every layer on its own named track, saved as `.aup3`.

## Files

| What | Where |
|---|---|
| References, cut to the passage the user named | `C:\Users\Felou\Music\SFX\Reference\` |
| Drafts for the user to hear, a new name per iteration — `Saved` may be wiped | `Saved/Audio/<Sound>/Draft/` |
| Source files that ship — 16-bit PCM, peak −1 dBFS, checked in | `SourceArt/Audio/<Family>/SFX_<Name>.wav` |
| An animation's score: its stems and `cues.json` | `SourceArt/Audio/<Animation>/` |
| Sound Wave assets | `/Game/Art/SFX/<Family>/` as `SW_<Name>` |
| A synth's fitted variants | a JSON beside its script |

## A Python synth

- One script per sound family, run with `uv run`, dependencies declared inline, indexed in `AI/Python/CLAUDE.md`.
- It writes each layer as its own WAV at mix balance, plus a mix preview; each layer has its own noise, so the
  layers add up to the mix.
- Band-split noise under per-band envelopes builds impacts, cracks, rattles, rooms and swells; a pitched body is
  a few sine partials; a loop is built on a circle (`GameSoundDesign.md`).
- A reference-matched synth doubles as a `match_reference.py` model — bounded parameters, the onset time, a render
  function — so the same file is fitted, compared and exported.
- Synthesis runs at 48 kHz with a low cut near 120 Hz unless the sound owns the low end.
- Each sound draws from its own random stream, seeded by its name: re-voicing one sound never re-rolls another.
- A sound that swells in starts before its strike: the file leads the beat by the swell, and its notify moves
  earlier by that lead.

## Reusable sounds

Every file below is one sound on its own, at −1 dBFS: reuse it before synthesising a new one, and set its loudness
on whatever plays it. Regenerate a family with its script.

| Sound | Assets in `/Game/Art/SFX/Metal/` | Character | Script |
|---|---|---|---|
| Crack | `SW_Metal_Crack` | The trailer clack's strikes alone: a four-click broadband snap, 150 ms — a part slamming home | `metal_one_shots.py`, from `hex_lock_layers.py` |
| Bright crack | `SW_Metal_CrackBright` | The clack heard first in the trailer, brighter and harder | same |

| Sound | Assets in `/Game/Art/SFX/Machine/` | Character | Script |
|---|---|---|---|
| Clamp slam | `SW_Machine_Slam_Low` / `_Mid` / `_High` | Heavy parts clipping together, a car-door ker-chunk: latch catch, the mass seating 22–29 ms later dead within ~60 ms, a brief rattle, ~0.3 s, largest part lowest — under a crack, at 0.7 of its volume | `heavy_machine.py`, written by `hex_intro_score.py ship` |
| Seal hiss | `SW_Machine_Hiss` | A pneumatic seal venting, 0.9 s — 70 ms after a clamp | same |
| Gear thunk | `SW_Machine_GearThunk_1`–`4` | One heavy tooth dropping into mesh: a dull clunk at 110–650 Hz, 0.25 s | same |
| Release | `SW_Machine_Blast` | A machine blowing open: a sub drop 70→26 Hz, a broadband strike, a huge low hull, 3 s — after a silent freeze | same |

| Sound | Assets in `/Game/Art/SFX/Mech/` | Character | Script |
|---|---|---|---|
| Tick | `SW_Mech_Tick_1`–`4` | A ratchet pawl: a low knock under a bright edge, 0.12 s | `mech_kit.py`, written by `hex_intro_mech_score.py ship` |
| Click | `SW_Mech_Click_1`–`4` | A chain link, dry and bright, 60 ms | same |
| Hit | `SW_Mech_Hit_1`–`4` | Metal struck: a clatter of snaps within ~45 ms, a smear and short rings, no body, 0.4 s | same |
| Impact | `SW_Mech_Impact_Low` / `_Mid` / `_High` | A heavy part landing: a hit over a low knock, then a few hits clattering, 0.7–1.2 s, largest part lowest | same |
| Riser | `SW_Mech_Riser` | A sting rising 1 s into a hard cut: sub swell, sparkle, a diving whistle | same |
| Grind | `SW_Mech_Grind` | Plates dragging in stick-slip bursts, 0.7 s | same |
| Air stutter | `SW_Mech_AirStutter` | An air tool stuttering four times, 0.66 s | same |
| Air blast | `SW_Mech_AirBlast` | Air blowing out broadband, loose parts clicking after, 1.2 s | same |
| Boom | `SW_Mech_Boom` | A sub hit at 38 Hz with a low noise body, no glide, 3 s | same |
| Tone tail | `SW_Mech_ToneTail` | Pure lines ringing on after a big hit, 2 s | same |
| Loops | `SW_Mech_Drone_Loop`, `_Hum_Loop`, `_Rattle_Loop`, `_Whir_Loop` | Seamless 4 s: the sub bed, a held hum, a chain rattle, a gear train running | same |

| Animation score | Stems in `/Game/Art/SFX/Enemy/HexBoss/` | Script |
|---|---|---|
| Hex boss intro | `SW_HexIntro_Bed` (the sub bed, dipping before each landing, dead at the freeze), `SW_HexIntro_Shiver_*` (clicks while each piece shivers), `SW_HexIntro_Whir_*` (a gear train per ring), `SW_HexIntro_Rattle` (the gather), `SW_HexIntro_Engine` (an electric engine idling from the start, racing to the release, winding down) or `SW_HexIntro_Hum` (a hum rising through the gather alone) — the score's `HUM_ON_MONTAGE` picks which goes on the montage | `hex_intro_mech_score.py ship` |

## Scoring an animation

A sound that follows an animation is timed off the animation's own motion, not placed by eye.

1. Dump the bones with `AI/Python/Anim/dump_bone_motion.py`: position, unwrapped yaw and scale, four samples a frame.
2. Score it with a script like `AI/Python/Audio/hex_intro_score.py`: it writes its one-shots, a stem per
   continuous sound, a `cues.json` of every notify (track, sound, time, volume) and a preview mix of the whole
   animation — into `Draft/`, or into `SourceArt/Audio` once approved.
3. The user judges the preview; the balance is the score's volume constants.
4. Import with `import_sound_waves.py`, then put the cues on the montage — `hex_boss_intro_sounds.py` does.

- A one-shot is a notify; anything denser than a few hits a second is a stem started by one notify, since a notify
  fires on the game frame and would jitter a fast train.
- A turning part meshes once per tooth it turns past; its tooth count sets the rate, a larger part carrying more.
- A turning part's motor follows its turn speed sample by sample, from the unwrapped yaw, through its inertia, and
  pulls harder while that speed climbs.
- A shaking part's width is the third difference of its position, which ignores the smooth motion under it; it
  sets how hard and fast the part strains.
- A part holding still before a hit is silent: the silence is what makes the hit land.
- Every part has a size, and all its sounds are divided by it: the largest ring is the lowest.
- A stem that only supports the picture, like a motor or gear train, is set back: quieter and duller.

## The loop

1. State the target as numbers: length, attack, centroid contour, band balance.
2. Build or fit, then measure the result — `listen.py`, `match_reference.py compare` against a reference hit, or
   `compare_sections.py` against a reference section.
3. Put a draft in `Draft/` and tell the user its path; only the user judges timbre.
4. Turn their words into a measure and a parameter (`ListeningToSound.md`), and iterate.
5. Once approved, write the `SFX_` file into `SourceArt/Audio/<Family>/` — from the script, or from Audacity with
   the layers on their own tracks and the `.aup3` saved beside it.

## Into Unreal

- `AI/Python/Asset/import_sound_waves.py` imports the `SFX_` files of each listed `SourceArt/Audio` folder as `SW_`
  Sound Waves through the editor, replacing in place; the folder and package pairs are a constant at its top.
  A Sound Wave there whose source file is gone is deleted unless something still references it, so removing a
  sound is deleting its `SFX_` file and re-running the import.
- Unreal takes 16- or 24-bit PCM only; a float WAV is converted first.
- An animation sound is a play-sound notify on the animation's beat, placed with
  `AI/Python/Anim/anim_sequence_authoring.py` — `hex_boss_intro_sounds.py` is one.
- Relative loudness lives in the notify's volume, not in the file; every file ships at the same peak.
- Saving an asset fails while PIE runs: ask the user to stop it.

## Rules

- Synthesis only: a reference is a target to measure, never material — no cut, filtered or resynthesised piece of
  it ships.
- Never mix and render: layers stay separate until export.
