# Sound Creation

The route for a game sound, from an intent or a reference clip to a Sound Wave playing on its beat. The craft —
layers, sound grammar, recipes — is `AI/MCP/Audacity/GameSoundDesign.md`; judging a sound by numbers and matching
a reference is `AI/MCP/Audacity/ListeningToSound.md`; driving Audacity is `AI/MCP/Audacity/CLAUDE.md`.

## Choosing the route

| The sound | Route |
|---|---|
| A simple tonal cue — a blip, a chirp, a sweep, a few layers | Audacity's generators, one track per layer |
| Noise textures, click trains, loops, per-band decays, stereo width | A Python synth in `AI/Python/Audio/`, one WAV per layer, the layers loaded into Audacity |
| "Make it sound like this" — a reference clip | `AI/Python/Audio/dissect.py` to take it apart, a patch for `sfx_patch.py`, `fit_patch.py` to fit and compare it (`ListeningToSound.md`) |
| A hit buried in a trailer's music | `AI/Python/Audio/match_reference.py`: cancel the backing between two copies, fit a Python synth |
| A new kind of sound, no reference yet | Fetch a sound-pack video of it with `fetch_reference.py` and reproduce one first |

- Audacity generates only tones, chirps, plain noise and an even click track; anything irregular, gliding or
  band-dependent is synthesised in Python.
- The agent judges by measurement, and a fit takes thousands of renders: that loop only runs in Python.
- Audacity stays the desk the user listens and balances at: every layer on its own named track, saved as `.aup3`.

## Files

| What | Where |
|---|---|
| References, cut to the passage the user named | `C:\Users\Felou\Music\SFX\Reference\` |
| A sound pack's references: full audio, video, `cuts/`, `labels.txt`, `survey.txt`, `README.md` | `C:\Users\Felou\Music\SFX\Reference\<Pack>\` |
| Reproduction patches — recipes fitted to a reference, synthesis only | `AI/Python/Audio/Patches/<Pack>/<Sound>.json` |
| A reproduction's synth, reference-then-synth A/B and picture | `Saved/Audio/Reproductions/<Sound>/` |
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
| Surge | `SW_Mech_Surge` | A machine coming alive whole: one heavy hit locking home over a deep knock and a low push of air, no debris, 1.5 s | same |
| Break | `SW_Mech_Break` | A machine blowing apart: a clattering impact, the boom, air blasting out with loose parts clicking, a ring left hanging, 3 s — for an outro | same |
| Loops | `SW_Mech_Drone_Loop`, `_Hum_Loop`, `_Rattle_Loop`, `_Whir_Loop` | Seamless 4 s: the sub bed, a held hum, a chain rattle, a gear train running | same |
| Engine | `SW_Mech_Engine_Loop` | The hex boss engine at cruise, seamless 3.5 s: a rotor buzz woofing 3.4 times a second — its running sound through the fight | same |

The ninja kit is air and cloth, never metal: a band of noise swept by its speed — brightest where loudest — and short
band-limited cracks, no ringing line anywhere.

| Sound | Assets in `/Game/Art/SFX/Ninja/` | Character | Script |
|---|---|---|---|
| Cut | `SW_Ninja_Cut_1`–`4` | A point stabbing out: a short air cut peaking early over a puff, 0.13 s | `ninja_kit.py ship` |
| Flurry | `SW_Ninja_Flurry` | Eight cuts a frame pair apart, brightening: every point firing round the star, 0.7 s | same |
| Swish | `SW_Ninja_Swish_1`–`4` | A blade swung past: air swelling to its peak and dying, the edge whistling faint an octave up, 0.25–0.3 s | same |
| Whoosh | `SW_Ninja_Whoosh_Low` / `_Mid` / `_High` | A big body swung round, 0.5–1 s, largest lowest | same |
| Flap | `SW_Ninja_Flap_1`–`4` | Cloth snapping taut: a few dry cracks within 35 ms over its body's pop, 0.12 s | same |
| Land | `SW_Ninja_Land` | A soft landing on a cushion of air, cloth settling, no knock, 0.45 s | same |
| Gust | `SW_Ninja_Gust` | Air blasting outward, darkening as it dies, a low push under it and a howl riding it, 1.8 s | same |
| Inhale | `SW_Ninja_Inhale` (1 s), `_Inhale_Short` (0.3 s) | Air drawn in, brightening into a hard cut: the file ends on the beat it leads | same |
| Vanish | `SW_Ninja_Vanish` | A smoke-bomb poof, the smoke's hiss left hanging, 1.6 s | same |
| Loops | `SW_Ninja_Wind_Loop`, `_Whirl_Loop`, `_Flutter_Loop` | Seamless 4 s: foliage swaying (leaf ticks over a leafy hiss and a breath of air), 8 swishes a turn at a turn a second, cloth flapping 11 times a second | same |

| Animation score | Stems in `/Game/Art/SFX/Enemy/HexBoss/` | Script |
|---|---|---|
| Hex boss intro | `SW_HexIntro_Bed` (the sub bed, dipping before each landing, dead at the freeze), `SW_HexIntro_Shiver_*` (clicks while each piece shivers), `SW_HexIntro_Whir_*` (a gear train per ring), `SW_HexIntro_Rattle` (the gather); the engine is the fight loop itself, started on the first frame and bent by the montage's curves — the score's `HUM_ON_MONTAGE` can put `SW_HexIntro_Engine` or `SW_HexIntro_Hum` on instead | `hex_intro_mech_score.py ship` |
| Hex boss fight loop | `SW_Mech_Engine_Loop`, `SW_HexFight_Whir_Loop` (the outer ring's tick-tock after its landing, 4 teeth a second) and `SW_HexFight_Rattle_Loop` (the gather's rattle at its loudest, silent at rest), their volumes and the intro's curves in `loop.json` | same |
| Hex boss abilities and death | No stems: kit one-shots on each montage's beats, and the fight loop bent by its curves, in `SourceArt/Audio/HexBossMontages/<montage>/` | `hex_boss_montage_score.py ship` |
| Star boss, every montage | No stems: ninja kit one-shots on beats read off the motion — an air cut on each point stabbing out, air drawn in to the still frames before a nova, a gust on it — and its fight loop (`SW_Ninja_Wind_Loop`, all but quiet at rest (−20 dB), lifted 14 dB by every montage, the one-shots 4 dB under their scored volumes; the whirl and flutter, silent at rest) bent by curves read off the spin and the shake, in `SourceArt/Audio/StarBoss/` | `star_boss_score.py ship` |

## Scoring an animation

A sound that follows an animation is timed off the animation's own motion, not placed by eye.

1. Dump the bones with `AI/Python/Anim/dump_bone_motion.py`: position, unwrapped yaw and scale, four samples a frame.
2. Score it with a script like `AI/Python/Audio/hex_intro_score.py`: it writes its one-shots, a stem per
   continuous sound, a `cues.json` of every notify (track, sound, time, volume) and a preview mix of the whole
   animation — into `Draft/`, or into `SourceArt/Audio` once approved.
3. The user judges the preview; the balance is the score's volume constants.
4. Import with `import_sound_waves.py`, then put the cues on the montage — `boss_montage_sounds.py` does — and a
   boss's loop layers on it with `boss_fight_loop.py`.

- A one-shot is a notify; anything denser than a few hits a second is a stem started by one notify, since a notify
  fires on the game frame and would jitter a fast train.
- A turning part meshes once per tooth it turns past; its tooth count sets the rate, a larger part carrying more.
- A turning part's motor follows its turn speed sample by sample, from the unwrapped yaw, through its inertia, and
  pulls harder while that speed climbs.
- A shaking part's width is the third difference of its position, which ignores the smooth motion under it; it
  sets how hard and fast the part strains.
- A part holding still before a hit is silent: the silence is what makes the hit land.
- Beats a clip computes rather than keys — an accelerating run of eruptions — are found in the dump, not copied from
  the clip's script: a bone stabbing out faster than a threshold, many on one frame a burst, the still run before it.
- A shape turned by a whole multiple of its own symmetry looks unturned, so its spin is read modulo that symmetry:
  two sequences meeting a point apart are no spin at all.
- Every part has a size, and all its sounds are divided by it: the largest ring is the lowest.
- A stem that only supports the picture, like a motor or gear train, is set back: quieter and duller.

## A sound running through a fight

A boss's running sound — its hum — is one continuous loop, never a hum per montage crossfaded: two takes of the same
tone beat against each other wherever they overlap.

- `UGeoLoopSoundComponent` on the enemy plays its `Loops`, every layer at once, from the fight's start to its end;
  a montage playing before the fight, like an intro, starts them with a `GeoLoopSoundNotify` on its first frame.
- A layer's rest level is its volume while no montage bends it: at −80 dB it is silent until a montage's volume
  curve raises it — by 80 to play it as authored — like the rattle a wind-up shakes.
- A montage bends the loop through float curves on the montage: semitones on each layer's pitch curve, decibels on
  its volume curve, 0 leaving it as authored. They reach the loop weighted by the montage's blend, so a montage
  whose curves start and end at 0 hands the loop back untouched. A layer held at −80 dB stays out.
- Every pitched part of a machine synth that scales with its speed makes a pitch curve a speed curve: the loop played
  faster is the machine turning faster. Only a fixed noise band, like the engine's air, slides with it.
- Between montages the pitch wanders slowly by the layer's drift; layers sharing a drift period wander as one.
- The fight's pitch and volume are tuned by ear on the boss, then copied into the score's `FIGHT_ENGINE`: the
  intro's curves lift the loop from them back to the intro's engine and glide down to them at its end. The drift
  stays on the boss.
- The score writes the curves from the speed it synthesised the old stem at, and renders the loop as Unreal plays
  it — curves, pitch clamp and all — against that stem, level and brightness per 100 ms.
- Pitch under 0.25 is clamped by the project's `GlobalMinPitchScale`.
- An ability montage's wind-up is stretched to the ability's delay and its live phase loops for as long as the ability
  runs, so everything continuous in it lives in the curves, which stretch and loop with the montage; notifies stretch
  too, but a stem would not, so an ability montage carries one-shots only.
- A looping section's curves hold the same value at its two ends, and stay near steady, since the ability can jump to
  its end section from anywhere in it; `star_boss_score.py` tilts each one across the section until they meet, and a
  hit's swell dies before the next section starts.
- A boss's death plays on past its fight, so the loop keeps playing while the boss is dead; the death's curves spin
  it up, and a `GeoLoopSoundNotify` set to stop cuts it on the blast, before the boss is gone.

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
- A file named `_Loop` is marked looping and to play when silent on import, so a curve holding it silent never
  restarts it; it is built seamless, never faded at its edges.
- Unreal takes 16- or 24-bit PCM only; a float WAV is converted first.
- An animation sound is a play-sound notify on the animation's beat, placed with
  `AI/Python/Anim/anim_sequence_authoring.py` — `boss_montage_sounds.py` is one.
- Relative loudness lives in the notify's volume, not in the file; every file ships at the same peak.
- Saving an asset fails while PIE runs: ask the user to stop it.

## Rules

- Synthesis only: a reference is a target to measure, never material — no cut, filtered or resynthesised piece of
  it ships.
- Never mix and render: layers stay separate until export.
