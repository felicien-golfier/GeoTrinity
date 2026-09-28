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
| Bike freewheel (`BikeFreewheel_Ref_Full.wav`) | A spinning wheel coasting down from 51 to 11 ticks a second, each tick equally loud at every speed. A tick is a 2 ms mono snap across 1.2–16 kHz over short hub partials at 1.9, 2.7, 4.5, 8.2, 10.4 and 14.2 kHz decaying in 4–8 ms, with a small knock below 350 Hz. The other pawl adds a faint tick 28 dB down, 28–64 ms after. A small machine's sound: under a heavy part it reads as a toy. |
| Turret power-down (`TurretDown_Ref.wav`, the game's own) | 5.4 s, stereo in anti-phase: a rumble at 30–250 Hz the loudest band throughout, a dozen inharmonic lines at 1–3.5 kHz 10–20 dB under it gliding down 20–30 % at different rates, a housing peak near 1.8 kHz and air to 10 kHz ~20 dB down; one long wind-down, the lines curving faster as it stops. |
| Electric car under load (`EVLoad_Ref.wav`) | Accelerating: one family of motor orders at 1×, 2×, 3× and an electromagnetic 4.7×, the first climbing 290→490 Hz over 2.2 s, each doubled by a second line ~6 % above that beats against it; a drive and road rumble at 63–125 Hz the loudest band, the orders 4–8 dB under it, nothing above 4 kHz. At cruise the orders sink and the rumble stays. The model for anything turning. |
| Mechanical intro (`MechanicalIntro\`, cut into 23 sections, anatomy in its `README.md`) | 71 s: a sub drone at 28–41 Hz loudest throughout, every mechanical layer 5–20 dB under it, the drone dipping before each hard entry. Metal is broadband snaps dead within 17–40 ms with one or two short ring lines, never a body or a bounce; hits come in flurries of 3–6 within ~300 ms; a running machine is a click train of 9–20 a second under steady high lines; an air tool stutters then blasts broadband. The model for the hex boss intro. |
| Metallic hit | 2.2 s: a 100 ms swell, a broadband strike whose centroid falls 5 kHz→300 Hz over 0.5 s, a sub below 60 Hz held throughout; a dense partial cluster dies within 0.5 s and leaves 1.86 kHz with its 2.0× and 2.7× partials ringing to the end. |

- A cinematic hit is announced by a 100–200 ms swell into the strike.
- A riser rises in brightness and density, not only in pitch; a tape stop lowers pitch, pulse rate and bandwidth
  together.
- A ring reads as metal through its inharmonic partial, here the 2.7×; harmonic partials alone read as a note.
- The body sits 15–20 dB under the transient, and every sound peaks near −1 dBFS.
- An animation sound lands on the animation's own beats — each slam, lock or blast is one hit — with its tail cut
  to the Sound grammar below.

## Weight and scale

- Weight is heard in pitch, not loudness: a lower sound reads as a heavier object, a louder one only as a nearer one.
- Size scales every resonance of a part down together; the same part twice as big rings an octave lower.
- Material is heard in how many cycles a resonance lasts: long for its pitch reads as thin metal or glass, short as
  a thick, damped or clamped mass.
- A few clear partials ringing long read as a bell or an empty bar; a heavy metal part is many dense modes dying
  fast over a thud.
- The thud is a sine at 50–200 Hz sagging a little in pitch as it dies; its clunk modes above 200 Hz carry it on
  small speakers.
- A heavy part settles: its strike is followed by one or two softer bounces 10–25 ms later.
- A big hit is layers that each own one band, their attacks aligned: sub or boom, low-mid punch, mid snap, tail.
- A hit sounds bigger after a dip in level or a swell into it; slowing it down, lowering it and a darker, longer
  tail make it bigger still.
- Saturation and one shared compression over the layers fuse them into one object.
- A sound set in the background is quieter and duller together — low-passed near 6 kHz, not only turned down.
- A machine made of small-machine sounds — clinks, bike ticks, bar rings, all above 1 kHz, sparse and short —
  reads as a toy whatever its timing; the same timing on low, dense parts reads as heavy.

## Heavy machines

A futuristic machine is heard through its parts, each its own layer: motors, gears, clamps, seals, the hull under
strain and the field holding it. `AI/Python/Audio/heavy_machine.py` builds each one.

| Part | Anatomy |
|---|---|
| Motor | An electric motor under load: a drive rumble at 35–170 Hz divided by the part's size, loudest, under one family of orders at 1×, 2×, 3× and 4.7× locked to the speed, the first at 35–100 % of ~480 Hz from rest to full speed, doubled by a second rotor 6 % sharp and 4 dB down; a trace of air. The orders swell up to 3.5 dB while the speed climbs and sink 6 dB while it coasts. Its pitch trails the speed by ~0.4 s of inertia, its level follows within ~30 ms. Unrelated whines each gliding at their own rate read as a turbine spooling, not a part being driven. |
| Gear tooth | A dull clunk of dense modes at 110–650 Hz dying in tens of milliseconds, a low knock and the barest metal edge; one per tooth turned past. |
| Strain | Stick-slip friction: pulses 18–60 a second, faster and harder under more strain, each ringing a large plate's low modes at 80–700 Hz. |
| Holding field | A 50 Hz band-limited sawtooth low-passed near 500 Hz, beating against a copy 0.6 % sharp, as loud as the strain. |
| Clamp | A ker-chunk, like a car door or two parts clipping together, ~0.3 s: the latch catches in a bright snap, the mass seats ~22 ms later — longer for a larger part — in an unpitched low knock and dense modes dead within ~60 ms, under the full latch; the panel rattles briefly after, and the air pushed ahead swells in just before the seat. A pitched sine sagging under it, or a low body ringing on, reads as a drum. |
| Seal | Bright air at 1–9 kHz swelling in over 25 ms and dying over a quarter second, ~70 ms after the clamp. |
| Release | The loudest moment: a sub dropping 70→26 Hz over most of a second, a broadband strike, a huge low hull and air debris, soft-clipped, ~3 s — after a dead-silent freeze. |

- Film designers slow real machines down for scale — a car door slowed 25 % becomes a giant robot's footstep;
  synthesis does the same by dividing every frequency by the part's size.
- Motion data drives the parameters directly: the turn speed sets the motor, the shake sets the strain, the
  squash sets the pressure.
- A machine spinning up rises in pitch, brightness and level together; one winding down falls in all three.

## Mechanical metal

The mechanical intro reference's parts, measured against its sections; `AI/Python/Audio/mech_kit.py` builds each
one, and `hex_intro_mech_score.py` scores the hex boss intro with them.

| Part | Anatomy |
|---|---|
| Bed | A sub at 26–46 Hz, loudest, with a bass band 55–165 Hz 3–5 dB under it and a faint air tail falling to −60 dB by 8 kHz. |
| Tick | A ratchet pawl: a low knock at 125 Hz as loud as its 4 kHz edge, the edge dead in ~10 ms above 8 kHz and ~35 ms at 2–4 kHz, short rings near 1.5, 4.7, 8.5 and 13.8 kHz; strong and weak in turn, ~9 a second. |
| Click | A chain link: a dry snap flat from 1 to 8 kHz, 250–500 Hz 13–19 dB down, dead in ~15 ms, often a second link a few milliseconds on. |
| Hit | A clatter of 4–7 snaps within ~45 ms, the first hardest, flat 1–8 kHz with 16 kHz 13 dB down, over a 2–8 kHz smear dying in ~200 ms and short rings at 1.25 and 2.1 kHz. |
| Impact | A landing: a hit over an unpitched low knock 4 dB under it, then 3–5 hits clattering within ~300 ms, each softer. |
| Gear train | A light tick per tooth with no knock, over a held comb of mid lines — harmonics 5–18 of ~110 Hz — steady lines at 2.9 and 5.4 kHz and a faint hiss, all rising with the speed. |
| Hum | Held lines at 521, 1230 and 2197 Hz, each beating 2 Hz against a copy, rising a quarter in pitch as the pressure builds. |
| Engine | An electric engine turning: a rotor buzz of 8 harmonics each 4 dB under the last, from ~42 Hz at rest to 120 Hz flat out, with the hum lines riding on it; once a turn a vane sweeps past in a woof — ~6 dB louder, the high orders swelling most, the pitch bending 1.5 % up then down, a puff of air at 80–900 Hz — 0.8 woofs a second at rest, 4 flat out. Pitch, woof rate and level all follow one speed, so it spins up and winds down. |
| Air | A stutter ~8 a second, each burst broadband over a harmonic comb on 1.2 kHz; a blast broadband to 16 kHz, darkening as it dies, loose parts clicking after it. |
| Riser | A sub swelling in, sparkle snaps 1–16 kHz thickening, a whistle at 10 kHz diving to 5.25 kHz and holding, a hard cut. |

- An animation scored this way dips the bed 0.2 s before each hard entry, rises through the build, falls dead
  silent for the freeze, and brings the bed back with the release.
- An engine idling under the whole animation, racing to the release and winding down only after it, ties the parts
  into one machine; it alone holds, flat out, through the freeze.
- The low knock under every metal event is what makes it heavy; the snap above it is what makes it metal.

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
