# Game Sound Design in Audacity

Building game sound effects from synthesis alone through the `audacity` MCP server. Read `CLAUDE.md` in this folder
first for connection, the selection model and the tool arguments that never reach Audacity.

## Working blind

- No tool result carries the sound itself; a sound is judged by numbers and a spectrogram until the user hears it
  — see `ListeningToSound.md`.
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
- Plain square and sawtooth alias into inharmonic partials that grow with pitch; above ~500 Hz a square body is a
  chirp with the `Square, no alias` waveform, and a sawtooth body stays low.
- Audacity also ships Pluck, Risset Drum and a Nyquist synthesis prompt, none of which the server exposes.

## Shaping

| Goal | Effect |
|---|---|
| Attack and decay | Fade in/out over a sub-selection; adjustable fade for a curved decay. |
| Pitch glide on any layer | Sliding stretch, up to ±12 semitones from selection start to end. |
| Transpose | Change pitch keeps length; change speed moves pitch and length together. |
| Filter sweep | Wah-wah at a frequency of one over twice the sound's length sweeps once across it. |
| Tone colour | Low/high-pass, bass and treble. |
| Grit | Distortion — its type is the only control that applies; hard clipping flattens the envelope, so fade after it. |
| Sputter | Silence short irregular regions of a layer; a filter change placed inside a gap steps the tone without a click. |
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
- Electricity is never a tone: it is a dense irregular train of broadband discharges, each a thin vertical stripe
  from the lows to 16–20 kHz, with a centroid at 7–10 kHz and a near-flat spectrum from 300 Hz to 15 kHz.
- An electric impact is ~0.5 s of that crackle over a heavy thump below 150 Hz, stopping abruptly, then a low
  rumble and a thin hum near 1 and 2 kHz decaying over seconds.
- No exposed generator makes an irregular impulse train; synthesise that layer as a WAV and import it —
  `AI/Python/Audio/electric_layers.py` writes an electric loop's layers.
- A discharge is a noise burst decaying over about half a millisecond, not a single-sample spike: at 2–3 thousand
  per second they overlap into the dense crackle, crest about 15–18 dB; spikes alone read thin and ticky.
- Static over an electric bed is sparse: short bright snaps a few times a second and an occasional sputtering fizz.
- The crackle bed sits far under the rest, about 24 dB; at equal level it swamps the snaps.
- A clean harmonic series whose brightness and pitch swell reads as a brass instrument, not as high voltage; an
  electric loop stays pitchless.

## Cinematic references

Measured from a cinematic sound pack, each cut in `Music\SFX\Reference\Cinematic_*.wav`; the numbers are targets
for the listening loop. Epic sounds like these belong mostly to cinematics and animations — an intro, a death, a
phase change.

| Sound | Anatomy |
|---|---|
| Suspense riser | 1.7 s: a ~30 Hz sub sine fades 25 dB over 0.8 s while a dense cluster of partials 50–150 Hz apart across 1.5–5 kHz swells 15 dB; a hard cut at 1.2 s, and one pure ~1.2 kHz tone pings at the cut and decays over 0.4 s. |
| Slow down | 2.4 s tape stop: one falling speed drives everything — low pitch 110→23 Hz, pulse period 35→160 ms, spectral ceiling 13→2 kHz — while the level swells over 1.6 s. |
| Charge | 2.4 s, mono: two cracks 25 ms apart ~20 dB above the body, then a hiss with nothing below 300 Hz and a ~17 Hz flutter, under one pure whistle gliding 4.2→9.9 kHz almost linearly. |
| Snap — camera adjustment | 215 ms: a buzzing ~70 Hz thump driven to flat tops, held 70 ms and cut dead; three ticks 14 ms apart, each a crack with a 2 kHz formant dying in ~5 ms; a 100 ms room tail. |
| Radio beep | Three near-sine 1.8 kHz beeps of 25, 25 and 50 ms at 48 ms onsets, harmonics 36 dB down; 99 % of the energy in 1–3 kHz, the telephone band that says "radio"; a ~100 ms echo trails. |
| Flash — camera flash | 1.8 s: a 200 ms swell rising 25 dB into a double strike 30 ms apart, broadband to 16 kHz over a 60–120 Hz boom, a sweep diving 10→1 kHz in 250 ms, then 1.3 s of wide reverb whose centroid falls 3.5 kHz→700 Hz. The model for a death or a mind-blown moment. |
| Trailer clack (`EpicImpacts_Ref_Full.wav`) | Four clacks over a music bed: a brighter first one that swells in over ~60 ms, then one sample heard twice ~960 ms apart and a copy 3 dB down. Each is a cluster of cracks within 5 ms and a second cluster ~33 ms later, broadband to the codec's 16.5 kHz with nothing of its own below ~150 Hz, over a knock gliding down to ~500 Hz and a room darker as it dies. The lock of a part snapping into place; fitted by `hex_lock_layers.py`. |
| Bike freewheel (`BikeFreewheel_Ref_Full.wav`) | A spinning wheel coasting down from 51 to 11 ticks a second, each tick equally loud at every speed. A tick is a 2 ms mono snap across 1.2–16 kHz over short hub partials at 1.9, 2.7, 4.5, 8.2, 10.4 and 14.2 kHz decaying in 4–8 ms, with a small knock below 350 Hz. The other pawl adds a faint tick 28 dB down, 28–64 ms after. `SW_Metal_RatchetTick_*` is matched to it. |
| Metallic hit | 2.2 s: a 100 ms swell, a broadband strike whose centroid falls 5 kHz→300 Hz over 0.5 s, a sub below 60 Hz held throughout; a dense partial cluster dies within 0.5 s and leaves 1.86 kHz with its 2.0× and 2.7× partials ringing to the end. |

- A cinematic hit is announced by a 100–200 ms swell into the strike.
- A riser rises in brightness and density, not only in pitch; a tape stop lowers pitch, pulse rate and bandwidth
  together.
- A ring reads as metal through its inharmonic partial, here the 2.7×; harmonic partials alone read as a note.
- The body sits 15–20 dB under the transient, and every sound peaks near −1 dBFS.
- An animation sound lands on the animation's own beats — each slam, lock or blast is one hit — with its tail cut
  to the Sound grammar below.

## Loops

- A loop is built on a circle — events past the end wrap to the start, filters and swells are periodic — so its
  last sample runs into its first; it is never faded at its edges.
- Audacity edits never wrap, so a loop's layers are synthesised outside it and imported.
- A pitched layer in a loop completes a whole number of cycles per loop, and any pitch bend averages to zero.
- A project at a different rate from its tracks resamples on export, which moves the peak; synthesise at the
  project rate, 44.1 kHz by default, and measure the exported file, not the tracks.

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
