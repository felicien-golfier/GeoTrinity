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
- A sound that swells in starts before its strike: the file leads the beat by the swell, and its notify moves
  earlier by that lead.

## Reusable sounds

Every file below is one sound on its own, at −1 dBFS: reuse it before synthesising a new one, and set its loudness
on whatever plays it. Regenerate a family with its script.

| Sound | Assets in `/Game/Art/SFX/Metal/` | Character | Script |
|---|---|---|---|
| Crack | `SW_Metal_Crack` | The trailer clack's strikes alone: a four-click broadband snap, 150 ms — a part slamming home | `metal_one_shots.py`, from `hex_lock_layers.py` |
| Bright crack | `SW_Metal_CrackBright` | The clack heard first in the trailer, brighter and harder | same |
| Latch click | `SW_Metal_LockClick_Low` / `_Mid` / `_High` | A bolt strikes, then seats 9 ms later over a ringing bar at 1.1, 1.4 or 1.75 kHz — the metal of a lock; under a crack, at half its volume | `metal_one_shots.py` |
| Clink | `SW_Metal_Clink_1`–`6` | A small loose part touching another: a soft bar ring at 2.35–4.15 kHz, ~0.6 s — trembling, settling, debris | same |
| Ratchet tick | `SW_Metal_RatchetTick_1`–`4` | One pawl drop of a bike freewheel, matched to a real one — the ticks of anything that turns | same |
| Rattle tap | `SW_Metal_Tap_1`–`6` | One dry knock of loose metal at 0.95–3.3 kHz, 90 ms — the grain of a rattle | same |

| Animation score | Stems in `/Game/Art/SFX/Enemy/HexBoss/` | Script |
|---|---|---|
| Hex boss intro | `SW_HexIntro_Ratchet_Outer` / `_Mid` / `_Core`, `SW_HexIntro_RattleBody`, `SW_HexIntro_RattleRings` | `hex_intro_score.py` |

## Scoring an animation

A sound that follows an animation is timed off the animation's own motion, not placed by eye.

1. Dump the bones with `AI/Python/Anim/dump_bone_motion.py`: position, unwrapped yaw and scale, four samples a frame.
2. Score it with a script like `AI/Python/Audio/hex_intro_score.py`: it writes a stem per continuous sound, a
   `cues.json` of every one-shot (track, sound, time, volume) and a preview mix of the whole animation.
3. The user judges the preview; the balance is the score's volume constants.
4. Import with `import_sound_waves.py`, then put the cues on the montage — `hex_boss_intro_sounds.py` does.

- A one-shot is a notify; anything denser than a few hits a second is a stem started by one notify, since a notify
  fires on the game frame and would jitter a fast train.
- A turning part ticks once per tooth it turns past; its tooth count sets the rate, a larger part carrying more.
- A shaking part's width is the third difference of its position, which ignores the smooth motion under it; a
  rattle is a few taps per reversal, as loud as the shake is wide.
- A trembling part clinks by chance each frame, likelier and louder as the tremble widens.
- Stems are built from the shared one-shots, pitched per part.

## The loop

1. State the target as numbers: length, attack, centroid contour, band balance.
2. Build or fit, then measure the result — `listen.py`, or `match_reference.py compare` against a reference.
3. Put a draft in `Draft/` and tell the user its path; only the user judges timbre.
4. Turn their words into a measure and a parameter (`ListeningToSound.md`), and iterate.
5. Once approved, write the `SFX_` file into `SourceArt/Audio/<Family>/` — from the script, or from Audacity with
   the layers on their own tracks and the `.aup3` saved beside it.

## Into Unreal

- `AI/Python/Asset/import_sound_waves.py` imports the `SFX_` files of each listed `SourceArt/Audio` folder as `SW_`
  Sound Waves through the editor, replacing in place; the folder and package pairs are a constant at its top.
- Unreal takes 16- or 24-bit PCM only; a float WAV is converted first.
- An animation sound is a play-sound notify on the animation's beat, placed with
  `AI/Python/Anim/anim_sequence_authoring.py` — `hex_boss_intro_sounds.py` is one.
- Relative loudness lives in the notify's volume, not in the file; every file ships at the same peak.
- Saving an asset fails while PIE runs: ask the user to stop it.

## Rules

- Synthesis only: a reference is a target to measure, never material — no cut, filtered or resynthesised piece of
  it ships.
- Never mix and render: layers stay separate until export.
