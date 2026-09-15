# Game Sound Design in Audacity

Building game sound effects from synthesis alone through the `audacity` MCP server. Read `CLAUDE.md` in this folder
first for connection, the selection model and the tool arguments that never reach Audacity.

## Working blind

- No tool result carries the sound itself; a sound is judged by numbers until the user hears it.
- Clip info gives each layer's start and end; the analysis tool gives peak, RMS, DC offset and clipped samples.
- The analysis tool measures by exporting the current selection to a temporary mono file, so select the sound first.
- Only the user can judge timbre: play the region through the transport and ask.
- State each sound's intent — length, pitch contour, layers — before building it, so the user judges against a target.

## Sources

| Source | Gives |
|---|---|
| Tone | Constant pitch and amplitude: sine, square or sawtooth. |
| Chirp | Pitch sweep plus a linear amplitude ramp — the only generator with a built-in envelope. Accepts triangle. |
| Noise | White (bright), pink (balanced), Brownian (dark, rumble). |
| Rhythm track | Evenly spaced clicks at a tempo. |

- A chirp with equal start and end frequency is a tone with an envelope; use it for any decaying note.
- Audacity also ships Pluck, Risset Drum and a Nyquist synthesis prompt, none of which the server exposes.

## Shaping

| Goal | Effect |
|---|---|
| Attack and decay | Fade in/out over a sub-selection; adjustable fade for a curved decay. |
| Pitch glide on any layer | Sliding stretch, up to ±12 semitones from selection start to end. |
| Transpose | Change pitch keeps length; change speed moves pitch and length together. |
| Filter sweep | Wah-wah at a frequency of one over twice the sound's length sweeps once across it. |
| Tone colour | Low/high-pass, bass and treble. |
| Grit | Distortion — its type is the only control that applies. |
| Space | Echo with a short delay; reverb small and mostly dry. |
| Motion | Phaser; repeat for a stutter; reverse for a swell. |

## Anatomy of a sound

- A sound is up to three layers, each on its own track: transient, body, tail.
- The transient is 5–30 ms of high-passed noise or a high blip; it carries the impact and the timing.
- A mechanical click — a gear tooth, a ratchet, a latch — is never noise: it is a few fixed-pitch sine partials
  struck together, each decaying on its own, the high ones quiet and short, the low ones louder and longer.
- A click gets its body from a short echo (around 50 ms, low decay), which stands in for the part's housing.
- The body is a tone or chirp; its waveform and pitch contour carry identity.
- The tail is the decay — a fade, a short echo or a small reverb — and is the first thing to cut in a dense scene.
- A falling pitch reads as a shot, a hit or a loss; a rising pitch reads as a pickup, a charge or a reward.
- Frequent sounds are the shortest and least tonal; rare sounds carry more layers and length.
- Lengths: UI 30–150 ms, shot 80–250 ms, hit 100–300 ms, pickup 150–400 ms, power-up 300–800 ms, explosion 0.5–1.5 s.
- A sound heard many times a second ships as three or four variants a semitone or two apart.

## Sound grammar

The rules of `AI/ArtDirection.md` carried into sound: waveform says **who**, pitch contour says **what**, length and
layer count say **how much**.

| Channel | Rule |
|---|---|
| Waveform — identity | Circle is sine, Square is square, Triangle is triangle or sawtooth, the hex boss stacks detuned square and sawtooth in a low register. |
| Contour — meaning | Damage falls, healing rises on a consonant interval, protection holds a steady low-mid tone. |
| Size — magnitude | A stronger version is longer, wider in pitch range or has one more layer — never a different waveform. |

- Synthesis only: no recorded, organic or foley material.
- Noise is only ever a transient or a filtered texture, never the body.
- Every sound snaps in under 10 ms and leaves by fading and low-passing together.
- Long reverb tails are excluded: in a bullet hell a tail masks the next cue.

## Recipes

Starting points; frequencies follow the classic sfxr presets.

| Sound | Layers |
|---|---|
| Shot | Chirp square 1200→300 Hz, amp 0.8→0.05, 0.15 s; 10 ms white-noise transient at 0.3, high-passed at 2 kHz. |
| Hit | White noise 40 ms; chirp sawtooth 600→100 Hz, amp 0.8→0, 0.15 s; hard overdrive for a heavier hit. |
| Explosion | White noise 30 ms; Brownian noise 0.8 s low-passed at 1.5 kHz with a curved fade out; chirp sine 90→35 Hz, 0.4 s. |
| Pickup | Square 988 Hz for 50 ms, then square 1319 Hz for 100 ms with a fade out. |
| Power-up / charge | Chirp square 200→1200 Hz, constant amplitude, 0.5 s; phaser for motion. |
| Heal | Sine chirps at 523, 659 and 784 Hz, 40 ms apart, 0.4 s decays; echo at 80 ms delay, 0.3 decay. |
| Shield | Square tones at 220 and 223 Hz together for a slow beat, 0.4 s, low-passed at 1.5 kHz, faded out. |
| Dash | Pink noise 0.25 s, 100 ms fade in and 150 ms fade out, one wah-wah sweep. |
| Boss telegraph | Sawtooth tones at 110 and 116 Hz, 0.6 s, curved fade in, medium overdrive. |
| UI confirm / back | Two 60 ms square blips a fourth apart, rising for confirm and falling for back. |
| Defeat | Chirp sawtooth 400→60 Hz, amp 0.8→0, 0.8 s, low-passed at 2 kHz. |
| Gear tooth | Sine chirps at a fixed pitch decaying to 0: 300 Hz over 120 ms at 0.6, 1356 Hz over 10 ms at 0.5, 3245 Hz over 20 ms at 0.35, 6543 Hz over 30 ms at 0.15; the alternate tooth uses 1100, 2700 and 5356 Hz; echo 50 ms at 0.25; teeth 100–150 ms apart. |

## Build order

**Never mix and render.** Every layer stays on its own named track, so the user can change, solo and listen to
each one; export mixes the tracks into the file without touching the project.

**Save before every export.** The `.aup3` is written first, every time, so each WAV has the project that made it.

1. One sound per project: export writes every track, and no exposed tool mutes a single track.
2. For each layer, add a mono track, name it after the layer, select only that track and its time region, then generate.
3. A region starting after 0 offsets that layer, which is how a body follows its transient.
4. Shape each layer on its own selection; balance layers with amplify on that track alone.
5. Read clip info to confirm every layer's start and end.
6. Fade the first and last 2–5 ms of each layer so the sound starts and ends on silence without a click.
7. Select all and analyze: the measurement exports the mix, so its peak is the file's peak.
8. Bring that peak to -1 dB by amplifying all tracks together by the same ratio.
9. Save the project as `.aup3` next to the export, so every layer stays editable — always before exporting.
10. Export mono WAV under a new name — export refuses to overwrite a file, so each iteration is a new version.
11. Play the finished sound through the transport so the user hears it before moving on.

## Into Unreal

- Unreal imports uncompressed PCM WAV, 16- or 24-bit, at any sample rate, and stores 16-bit.
- An exported file dropped under `Content/` still needs an Unreal import to become a Sound Wave asset.
- Relative loudness between sounds is set in the engine, not baked into the file; every file ships at the same peak.
