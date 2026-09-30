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

## Cinematic references

Measured from a cinematic sound pack, each cut in `AI/Audio/References/Cinematic_*.wav`; the numbers are targets
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

## Game SFX, reproduced

Sounds from commercial game packs, each reproduced by a fitted patch in `AI/Python/Audio/Patches/<Pack>/`
(`ListeningToSound.md`, Reproducing a sound with a patch); the anatomy is the fit's own values. Each pack's
`README.md` in `AI/Audio/References/` holds its source and the rest of its sounds.

| Sound | Anatomy |
|---|---|
| UI beep, sine and square | A sine with a square 3.5 dB under it on the same 110 Hz, the square low-passed at 400 Hz, 24 dB per octave: the odd harmonics fade out past the fifth; 2 ms attack, 120 ms held while sinking 2.4 dB, 50 ms release. Reproduced under its own noise floor. |
| UI zap | One pure sine: 11 kHz diving to 350 Hz in 3.5 ms, then rising exponentially to 16 kHz over 42 ms at an even level, cut in 8 ms. The dive is the click, the rise is the zap. |
| Jump | A square wave gliding up exponentially 480 → 1400 Hz over 170 ms, falling 23 dB straight in dB, cut hard at 150 ms. |
| Heavy blaster shot | A broadband crack over 800 Hz for 180 ms; a saw pew diving 1.5 kHz → 360 Hz in 140 ms, dead within 75 ms; a sine boom 180 → 60 Hz in 50 ms, held 0.66 s; a low-mid noise body round 330 Hz ringing 0.7 s; a faint whine at 6.8 kHz 30 dB down. |
| Heal | Bell voices climbing over 0.7 s — A4, F5, G5, A5 together, then B♭5, F6, C♯6, G♯6 — each partials at 1× falling 60 dB in 3.2 s, 3.1× 12 dB under it in 1.5 s and 5.7× 4 dB under it in 0.8 s, struck in 1.4 ms: the upper partials sparkle and die, the fundamentals hang on as a chord. |
| Stat up | A sub thump (noise under 175 Hz, 0.3 s); a noise band rising 670 Hz → 2.2 kHz over 0.46 s with a 9 Hz flutter, cut dead at 0.6 s into a 0.9 s room. |
| Sci-fi bomb impact | A bright swell rising 40 dB over 110 ms into the hit; a rumble under 140 Hz rolling 0.66 s and gone by 2 s, the loudest band; a blast over 2 kHz falling 3 dB per octave, dying 50 dB over 1.2 s; metal rings at 1.9, 2.4, 3, 6 and 8 kHz struck 75 ms after the hit, gone in half a second. |
| UI click | 40 ms: a triangle chirping 450 → 1800 Hz in 16 ms, low-passed at 1.6 kHz, 40 dB down 29 ms later, over a dull noise tick under 1.5 kHz 8 dB lower. |
| Chip arpeggio, three notes up | A near-square pulse stepping every 31 ms through the notes and their lower octaves — 285, 134, 343, 166, 696 Hz — landing on 357 Hz and falling 14 dB over 0.35 s, into a 1.2 s room 11 dB down. |
| Laser pew | A saw diving ~2.5 kHz → 700 → 207 Hz in 0.14 s, its lowpass closing 13 kHz → 1.9 kHz over 0.16 s, held then gone by 0.75 s; a 5 ms crack over 4.8 kHz; a low body under 620 Hz; a 1.1 kHz noise band an octave wide 6 dB down, ringing to 0.6 s. |
| Force field hum | A buzz on 50 Hz wandering ±5 %, its harmonics low-passed under ~250 Hz, the mids 20–30 dB down, a slow comb sweeping the 250–1000 Hz harmonics about three times a second. |
| Arcade death (Geometry Dash) | Noise under 190 Hz struck in 8 ms, held 0.3 s while sinking 6 dB, gone 1.4 s later; a narrow noise band at 1.6 kHz 11 dB down dying over 1.2 s; mono. |
| Enemy tone (Geometry Wars) | 0.5 s on 334 Hz: its 2nd harmonic as loud as the first, the 3rd 18 dB down, the 6th 24 and the 9th 35; a 34 ms attack, sinking 6 dB to a 25 ms cut. |
| Enemy tone with a dome (Geometry Wars) | Two voices: a triangle falling 338 → 255 Hz over 0.23 s under a 26 Hz tremolo, and a square on 298 Hz 6 dB over it whose resonant lowpass opens 570 Hz → 10.6 kHz by 0.33 s and closes to 5.8 kHz, drawing its harmonics up and back down; all sinking 36 dB over 0.5 s. |
| Enemy wobble (Geometry Wars) | A saw low-passed at 1.3 kHz swooping 158 → 497 Hz in 0.16 s, back to 248 Hz by 0.54 s and down to 63 Hz, pulsing 41 times a second 10 dB deep, 40 dB down by 0.54 s. |
| Sword swish | Noise tilted -1.2 dB per octave, jumping to -19 dB in 20 ms, swelling straight in dB to its peak over 190 ms, falling 15 dB in 230 ms and fading over 330 ms more; its lowpass opens 2.7 → 3.3 kHz to the peak and closes to 1.1 kHz after it. |
| Retro pickup | A square root and a saw fifth 4 dB over it: E5 + B5 for 130 ms, then G5 + D6 for 250 ms, flat, cut in 23 ms, low-passed at 8–11 kHz. |
| Retro power-up | One saw sweeping 318 → 1594 Hz in 107 ms, four times over, each sweep sagging 3 dB before the next restarts it. |

- An arcade or UI sound is one oscillator whose pitch envelope is the whole gesture: a V for a zap, a rise for a
  jump or a pickup, a fall for a shot; its waveform is only its colour.
- A blaster is a stack, each layer one job: the crack is the timing, the pew the identity, the boom the weight, the
  body the size, a whine the energy.
- A heal is a chord built by an arpeggio: each bell rings on, so the notes pile up; the strike partials above 3×
  are what sparkle, and they die first.
- A buff is noise rising in a band with a flutter, over a thump; its hard stop before a room is what makes it a
  cast rather than a wind.
- An explosion swells before it hits, and its low end outlives everything else by a second.
- A laser's parallel falling curves are one harmonic-rich tone diving, not many voices; the lowpass closing behind
  the dive is what makes it read as travelling away.
- A chip arpeggio leaps octaves between its notes every 30 ms; the leaps are the sparkle, the landing note the
  meaning.
- A shield holds a low buzz and moves it slowly — a comb or a flutter — rather than sounding a pitch.
- A reward lands on an open fifth and its octave — C, G, C — held under bright partials: a glide up into it is the
  anticipation, a thump under its start the weight (Geometry Dash's level complete and achievement).
- A swish swells for about 190 ms and fades for twice as long, brightest at its peak; a low end as loud as its mids
  gives a blade its body, and a band around 2 kHz alone sounds like thin air.
- A retro pickup is two notes a third apart, each an open fifth; a retro power-up is one sweep repeated faster than
  a beat; a retro bomb is a dive cut dead by noise.
- Magic is noise with a sparkle: steady inharmonic lines above 5 kHz over a low swell flickering like a fire, wide
  in stereo where every arcade sound is mono.
- A menu's confirm is a quick arpeggio of pure notes rising, each quieter, with an echo; its error is a low buzz
  stuttered; a click is one tone gliding; a hover is a tick of noise.

## Designers' methods

Taken from sound designers explaining their own game sounds.

- An arcade shoot-'em-up's enemies die in tiny melodies, not booms: a flurry of notes starting low, jumping, then
  falling in an arpeggio, written slow and played about eight times faster; a screen full of them stays clean.
- The player's constant shot is the shortest and quietest sound of the game — a saw and a plucked note.
- A sci-fi power-up is three oscillators detuned apart so they never form a chord, one metallic one dropped about
  2.5 octaves as the machine's rumble, all rising ~18 semitones together over the attack and holding there; unison
  voices spread in pitch make it a machine rather than a note, and a 4-pole lowpass takes the shine off. Its length
  is its attack.
- A laser is a saw chirp 1200 → 50 Hz in 0.2 s doubled a few milliseconds late, so the copies comb as the pitch
  falls, under a slow deep phaser and a low-passed noise body; played faster it is a pistol, slower a cannon.
- A whoosh gains weight from layers that each own a band, not from one deeper whoosh.
- One game's sounds come from one family — the same waveforms, envelopes and space — so the set reads as one world;
  a sound borrowed from another style stands out as foreign.
- A sound repeated often gets three to twenty variations, never the same one twice in a row; a machine or a gun
  stays one sound, since a machine does not vary.
- A pickup may repeat unchanged, and rising in pitch with each one picked in a row it feels better still.
- A hit that does nothing — on a shield, on an immune enemy — gets its own weak, bouncing tink, which tells the
  player and makes the hits that land sound harder by contrast.
- A sound fits its action: as long as its animation, as heavy as its impact, and on its frame.
- A family of enemies can grow from one seed sound, each stretched, pitched and crushed its own way: they differ
  and still belong together.
- A treatment kept for the effects alone — bit reduction on every effect and never in the music — sets them apart
  from the music, so they read over it.
- An object repeated across a room sounds once, however many there are.
- A reward's cheer matches the world's mood: a dark game's fanfare stays short of happy.

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

## Electricity

Measured from a spell pack's electric one-shots and sci-fi zaps (`AI/Audio/References/ElecBank/`).

- Electricity is irregular everywhere: no layer of it repeats a cycle. Its core is noise, not a tone — a spell
  pack's electric shocks put 1–2 % of their energy in partials.
- Its lows are lurching noise, the waveform jumping between levels at uneven 2–10 ms steps, never a hum: a
  periodic buzz or a sine body under a crack reads as a knock on a table.
- Its highs are a hash of discharges, 300–1000 a second, clumping into bursts 15–40 ms long at uneven gaps, the
  level stuttering 16–28 times a second, 4–6 dB deep.
- Above 2 kHz is as loud as below 300 Hz; a discharge is broadband, so the lows and the hash move together.
- Arcs whistle through it: narrow lines at 1.5–8 kHz gliding up or down an octave over 30–100 ms, a few per
  sound, 10–15 dB under the hash — the "zzip" that names it electric.
- It swells in over 10–50 ms rather than striking: an electric hit is a burst of hash, not a click and a thud.
- A clean, steady harmonic series whose brightness and pitch swell reads as a brass instrument, not as high voltage.
- An electric impact is ~0.5 s of crackle over a heavy thump below 150 Hz, stopping abruptly, then a low rumble and
  a thin hum near 1 and 2 kHz decaying over seconds.
- A crackle is a dense irregular train of broadband discharges, each a noise burst decaying over about half a
  millisecond: at 2–3 thousand per second they overlap, crest about 15–18 dB; single-sample spikes read thin.
- No exposed generator makes an irregular impulse train; synthesise that layer as a WAV and import it —
  `AI/Python/Audio/electric_layers.py` writes an electric loop's layers, `zap_kit.py` its small hits.
- Static over an electric bed is sparse: short bright snaps a few times a second and an occasional sputtering fizz.
- The crackle bed sits far under the rest, about 24 dB; at equal level it swamps the snaps.
- A small electric hit is ~250 ms of that: hash and lurching lows swelling in within ~5 ms, four to eight clumps
  shrinking, one or two arcs gliding through, a few sparks after. Shorter than three clumps, it stops reading as
  electric.

## Satisfying hits

A hit satisfies when it confirms the action on the frame, has weight, and resolves cleanly, every time without
tiring the ear.

- The transient lands on the frame: full level within 1–3 ms, the loudest instant of the sound. A slow attack
  reads as late.
- Weight comes from a short body at 80–200 Hz under the transient, dying within ~60 ms; a hit with only highs
  reads as a tick, one with a long low tail as mud.
- The body is made of the sound's own material — noise lows for a noise sound, a low mode for struck metal; a
  clean sine thump under something else reads as a separate knock on a table.
- It is front-loaded and decays in one clean shape — loudest first, each part quieter than the last, no later
  bump louder than its start — and ends settled, not cut.
- One characteristic element carries its identity (a buzz, a ring, a crunch) and stays audible after the
  transient; a hit made only of a crack and a thump is generic.
- Every layer owns its band — sub, body, snap, sizzle — with their attacks aligned, so the stack reads as one
  object; overlapping layers mask each other into mush.
- Saturation gives density and loudness at the same peak and lets a small hit read on small speakers; soft
  clipping the mix fuses the layers. Too much flattens every hit to the same size.
- A short tonal element — a ping, a zing, a ring an octave or two above the body — reads as confirmation; pure
  noise confirms less.
- A hit heard several times a second is short (100–250 ms), sits a little under the shot that caused it, and
  darker than 5 kHz centroid; brightness and length are what make it tiring.
- Hits in a row vary — three or four variants, a semitone or two apart, never the same twice running — or the run
  sounds like a machine gun of one sample.
- Contrast makes a hit: a dip before it, or a weaker sound for hits that do nothing, makes the real one heavier.

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
| Engine | An electric engine turning: a rotor buzz of 8 harmonics each 4 dB under the last with the hum lines riding on it; once a turn of its big ring a vane sweeps past in a woof — ~6 dB louder, the high orders swelling most, the pitch bending 1.5 % up then down, a puff of air at 80–900 Hz. The rotor turns 30 times per woof, so pitch and woof rate are one speed: 1 woof a second and 30 Hz at rest, 4 and 120 Hz flat out. A loop closes on a whole number of woofs, a multiple of 4 so every hum line closes too. |
| Air | A stutter ~8 a second, each burst broadband over a harmonic comb on 1.2 kHz; a blast broadband to 16 kHz, darkening as it dies, loose parts clicking after it. |
| Riser | A sub swelling in, sparkle snaps 1–16 kHz thickening, a whistle at 10 kHz diving to 5.25 kHz and holding, a hard cut. |

- An animation scored this way dips the bed 0.2 s before each hard entry, rises through the build, falls dead
  silent for the freeze, and brings the bed back with the release.
- An engine idling under the whole animation, racing to the release, held flat out through the freeze and easing
  back after it to a cruise, ties the parts into one machine; the same engine looping at that cruise is the boss's
  only sound through the fight.
- A machine coming alive lands whole: one heavy hit over a deep knock and a push of air. A clattering flurry, air
  hissing out with loose parts clicking and a ring left hanging read as it breaking.
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

Starting points; frequencies follow the classic sfxr presets. Where a sound in Game SFX, reproduced covers the same
role, its measured anatomy and its patch are the better start.

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
