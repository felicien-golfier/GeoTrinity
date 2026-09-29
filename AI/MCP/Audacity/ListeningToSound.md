# Listening to Sound

How audio models hear and make sound, and how a text-only agent borrows their methods to judge the sounds it
builds in Audacity. Read `GameSoundDesign.md` for the craft; this file is about the ear.

## How an audio model hears

- A waveform is 44,100 numbers per second: too long and too raw for any model to read directly.
- Every model first turns it into time against frequency: a short-time Fourier transform over ~25 ms windows
  every ~10 ms.
- The frequency axis is warped to the mel scale, spaced the way the ear spaces pitch, and levels are taken in
  decibels, the way the ear hears loudness — the log-mel spectrogram.
- A classifier reads that spectrogram as an image: a CNN (PANNs) or a vision transformer over 16×16 patches (AST).
- A codec model tokenises instead: an encoder downsamples to 12–75 frames per second and residual vector
  quantisation stacks 8–32 codebooks, each coding what the previous one missed.
- In a codec the first codebooks carry *what* the sound is and the later ones *how* it sounds.
- Language enters through CLAP: an audio encoder and a text encoder trained so matching pairs land close in one
  embedding space; "does this sound like a laser" is the cosine similarity of the two embeddings.
- An audio-language model (Qwen2-Audio, Gemini) puts an audio encoder in front of an LLM; it captions and answers
  questions about sound, but fine acoustic judgement stays weak — 50–70 % on expert benchmarks.

## How an audio model makes sound

| Family | Mechanism | Examples |
|---|---|---|
| Autoregressive | Predicts codec tokens one after another like text; the codec decoder renders them. | AudioLM, AudioGen, MusicGen |
| Latent diffusion / flow | Denoises a VAE latent from noise, conditioned on a text embedding and a duration; the decoder renders it. | AudioLDM, Stable Audio Open, TangoFlux |
| Synthesizer programming | Searches the parameters of a real synthesizer until its output matches the prompt. | DDSP, CTAG |

- None of these compares samples: phase is inaudible, so reconstruction is scored on spectrograms at several
  window sizes at once — the multi-resolution spectral loss.
- A discriminator network trained to tell real from generated audio supplies the rest of "sounds right".
- Text alignment is tuned by ranking candidates with CLAP and training towards the preferred one.
- Quality is measured as Fréchet Audio Distance between embedding distributions; it only tracks human judgement
  when the embedding suits the domain — environmental-sound embeddings (PANNs, CLAP) do, the original VGGish does not.
- The final judge in every paper is a human listening test.

## Synthesizer programming — the method that fits

The Audacity layer model is the same decomposition the research uses: DDSP builds sound from a harmonic
oscillator bank, filtered noise and reverb — body, transient, tail.

- CTAG drives a 78-parameter modular synth (oscillators, ADSR envelopes, LFOs, noise) with an evolutionary
  search that maximises CLAP similarity to the prompt, about 300 rounds of 50–100 candidates.
- Its results are abstract "audio sketches", yet listeners identified them almost as often as a neural
  generator's (56 % against 59.5 %) and rated them more artistic.
- A sketch stays recognisable when its envelope, pitch contour and noisiness match the real sound, not its detail.
- An LLM with no audio input predicts effect parameters zero-shot (LLM2Fx); it does best given the audio's DSP
  features as text, the effect's DSP code and a few examples.
- An agent driving Audacity is this setup: its ears are measured features, a spectrogram picture, an optional
  CLAP score, and the user.

## What the ear rests on

Timbre research places most of what separates two sounds on three axes: attack time, spectral centroid and
spectral change over time. Pitch, loudness, noisiness and length complete the picture.

| Measure | The ear hears | Reading |
|---|---|---|
| Attack 10–90 % | Impact | Under 5 ms snaps; 20–100 ms swells. |
| Spectral centroid | Brightness | Its contour is the sound's opening or closing; a falling centroid darkens. |
| Strongest partial over time | Pitch contour | Falls for damage and loss, rises for reward (`GameSoundDesign.md`). |
| Flatness | Tone against noise | 0 is a pure tone, 1 white noise; a body stays under ~0.1. |
| Crest factor | Punch | High crest is transient-led; low crest is dense and sustained. |
| Decay to −20 dB | Size | Longer decay reads as bigger or further away. |
| First and last sample | Clicks | A non-zero edge is a broadband click. |

- The ear is most sensitive at 2–5 kHz: energy there reads loud and, in excess, harsh.
- Below ~100 Hz a tone needs far more level to sound as loud, and small speakers do not play it; a low body needs
  harmonics above 200 Hz to be heard on a laptop.
- Two sounds sharing a band mask each other; sounds that play together keep apart in frequency.
- A plain square or sawtooth folds every harmonic above half the sample rate back down as inharmonic partials —
  aliasing, heard as a dirty ring; it grows with pitch and no filter removes it afterwards.

## The listening loop

1. State the target as numbers before building: length, attack, centroid start to end, pitch contour, flatness.
2. Build the layers in Audacity (`GameSoundDesign.md`), save, export.
3. Run `uv run AI/Python/Audio/listen.py <file.wav>`; it prints the measures above and writes a picture beside
   the WAV.
4. Compare the printout with the target, then read the picture.
5. Change the one parameter the largest mismatch points to; repeat.
6. Play it for the user and turn their words into measures with the table below.

| User says | Measure | Fix |
|---|---|---|
| Bright, thin, harsh | Centroid high; energy at 2–5 kHz or aliasing | Low-pass, band-limited waveform, lower the partials |
| Dull, muffled | Centroid low | Raise the high-pass transient, open the low-pass |
| Muddy, boomy | Energy at 150–500 Hz | High-pass the layers that do not own the low end |
| Soft, no punch | Slow attack, low crest | Sharper transient, shorter fade in |
| Clicky | Non-zero edges | 2–5 ms fades at both ends |
| Too long, washes out | Late audible end, slow decay | Shorter fade, cut the tail |
| Buzzy, cheap | Square or sawtooth body, aliasing | Band-limited square, sine layers, low-pass |
| Wrong pitch or direction | Partial contour | Chirp end points, sliding stretch |
| A bell, an empty bar, tinny | Few partials ringing long for their pitch | Many dense modes, damped; a thud under them (`GameSoundDesign.md`, Weight and scale) |
| Not heavy, not epic | Centroid high, nothing below 200 Hz | A sagging low thud, a lower pitch, a dip or swell before the hit |
| A toy | Every layer above 1 kHz, sparse partials, short | The same timing on low, dense parts (`GameSoundDesign.md`, Heavy machines) |
| A drum, a tambour | A pitched low partial gliding down, a body ringing past ~150 ms | Unpitched noise knock, modes dead within ~60 ms, a latch click on top |
| Too present, should be background | Level and highs | Lower and low-pass together |

## Reference sounds

- A reference the user points to is the fastest target: measure it with the same script and match its numbers.
- A sound-pack preview video is a library of references: `uv run AI/Python/Audio/fetch_reference.py fetch <id> <Name>`
  writes its audio at 48 kHz and a 720p video into `Music\SFX\Reference\<Name>\`, cuts it wherever it falls
  silent and draws a contact sheet of the frame under each cut.
- Silence is measured against the montage's own floor, so a video playing its sounds over a music bed still cuts.
- A demo that shows each file's name on screen is labelled by `labels` with a crop around the name: the name's
  changes split the audio, each span starting on its sound's onset, and OCR reads the name; `name` then cuts the
  spans from the corrected `labels.txt`.
- A demo shows a name up to ~0.4 s before playing it; a span shorter than 0.1 s is a name flicked past.
- A name in a small pixel font needs the 720p video and a crop of that line alone; OCR confuses O/0, U/V and I/1.
- A montage with no names and no silences — a trailer, a drone pack — is cut by hand from its spectrogram with
  `"from to Label"` lines.
- Each cut is measured on its own; a whole-montage printout averages unrelated sounds into nonsense.
- `dissect.py survey <folder>` measures a whole pack in one line per sound, into the pack's `survey.txt`.
- The script's 1024-point spectrogram has ~47 Hz bins, too coarse below 200 Hz: pitch a sub with an 8192- or
  16384-point transform.
- Stereo is measured as mid against side and the correlation of left and right: a reverberant tail sits near
  0.7, a dry synth layer at 1.0.
- A file whose channels are in anti-phase folds to near silence in mono; `listen.py` then measures left minus
  right and says so.

## Taking a sound apart

`uv run AI/Python/Audio/dissect.py <file.wav>` cuts the sound off any bed around it and prints its anatomy — what a
patch is written from — with a picture of the partials over the spectrogram, the envelope and the noise per octave.

| Section | Reads |
|---|---|
| Envelope | Attack from −20 dB to within 3 dB of the peak, how long it holds there, falls to −20 and −40 dB. |
| Events | Onsets from the spectral flux: a burst, a stutter, a repeat, with the gaps between them. |
| Dominant lines | One tone holding the sound, pitched every millisecond on a 5 ms window: zaps, lasers, jumps. |
| Partials | Lines tracked on a 21 ms window with their pitch at five points, glide ratio and shape, level and fall. |
| Families | Partials at whole multiples of one: which harmonics and how fast they fall per octave of harmonic number. |
| Noise | What is left once the partials are masked, per octave: level, peak time, rise, fall. |
| Modulation, echo, stereo | Tremolo and vibrato rates and depths, a repeating delay, left-right correlation. |

- A tone sweeping faster than about an octave in 20 ms smears across a 21 ms window into what reads as noise: only
  the dominant lines follow it, and a sound under ~100 ms is read on a 5 ms-window picture before anything is written.
- The dominant lines stop at 500 Hz: under it a 5 ms window cannot tell a tone from low noise; the partials cover it.
- A family's fall per octave names the waveform: 6 dB for a saw (every harmonic) or a square (odd only), 12 dB for
  a triangle, faster for a filtered wave or a soft-clipped sine.
- A cut from a demo holds the demo's music bed: steady partials and a low noise floor under every sound belong to
  it, not to the sound.

## Reproducing a sound with a patch

A reproduction is a patch — a short JSON of layers, the sound designer's own controls — whose free values are
fitted to the reference; a patch that fits is a recipe that can be varied and shipped, since it is synthesis only.

1. Read the reference's anatomy and a fine picture of it, and write the patch in `AI/Python/Audio/Patches/<Pack>/`:
   one layer per part heard, each free value `{"fit": [value, low, high]}` started where the picture puts it.
2. `uv run AI/Python/Audio/fit_patch.py fit <patch.json> [renders] [searches]` fits them by CMA-ES and writes them
   back; each further search starts again round the best so far, which a patch of many layers needs.
3. Read the printout and `compare.png`, add or reshape the layer the gap points to, fit again.
4. The user listens to `reference_then_synth.wav` in `Saved/Audio/Reproductions/<patch>/`, or to every
   reproduction at once in the file `fit_patch.py playlist` writes there.

- The synth, `sfx_patch.py`: oscillators (sine, triangle, and saw, square and pulse band-limited by PolyBLEP), FM
  pairs, inharmonic partials with their own decays, noise, blips at random times and pitches (chatter, sparkle,
  glitch), each through a lowpass, a highpass and a resonant bandpass moved frame by frame; step envelopes on
  pitch, level and filters; drive, echo, a small room, bit and rate reduction. A render takes tens of
  milliseconds, so a fit of a few thousand renders runs in minutes.
- A reference is `file.wav@start@end` to fit one span of a cut: one shot out of a run, or a sound off a bed too
  loud to be told from it.
- A sound of random parts — chatter, sparkle, crackle — fits on its statistics: its rate, range and level; its
  distance stays well above a tonal sound's. Chatter the ear follows as a figure, the same in every repeat, is
  notes written out on a stepped pitch envelope: blips only fit a texture heard as random.
- A tremolo is compared frame by frame, so out of phase with the reference it only costs the fit, which then turns
  it off: its phase is fitted with its rate and depth. A wobble in a tone can also be two voices a few hertz apart
  beating, which the tracker shows as two lines where one is expected.
- An envelope's steps are durations, like an ADSR, so a fitted time can never fold it back on itself.
- A value shared by several layers — a pitch, an envelope, a bell's partial ratios — is written once in the patch's
  `envelopes` and named `"@name"` where it is used.
- The distance is the mean decibel gap between the two log-mel spectrograms at 256, 1024 and 4096-sample windows,
  level-matched, plus 6 dB for every octave the brightness contour strays: the contour gives a misplaced pitch or
  sweep a slope back to its place, where the spectral gap alone gives it none.
- Nothing counts 50 dB under the reference's loudest cell, nor under the bed measured before its onset: a codec's
  haze around a pure tone and a demo's music are not the sound.
- Both sounds start on their onset to half a millisecond; a smoother onset lands earlier than a synth's first
  sample and costs the fit a whole attack.
- The synth is cut as far under its peak as the reference's cut reaches, so a swell rising out of the reference's
  bed starts at the same point on both; a synth whose loudest layer is wrong starts elsewhere, and the gap shows it.
- The synth against itself on another noise seed is the distance it cannot go under: a noisy sound sits a few dB
  above it when it fits, a pure tone near zero.
- A value pinned at its bound means a layer is doing another's job or one is missing; a block of one colour in the
  difference picture is where.
- The fit refines a patch; it does not find a pitch trajectory it was not started near — read the line off the
  picture first.
- A filter sweep is a resonant band with skirts falling 6 dB per octave; a band with no skirts leaves the rest of
  the spectrum empty and the fit widens it to compensate.

## Matching a reference hit

A synth matches a reference hit when its band envelopes do: third-octave power every millisecond from 160 Hz to
16 kHz, and the energy above 3 kHz every half millisecond for the attack.
`uv run AI/Python/Audio/match_reference.py` runs the loop — onsets, repeats, target, fit, compare.

1. List the hits with `onsets`, then run `repeats` on them to find copies of one sample.
2. Build the target with `target`: `pair` from two copies of one sample, `single` from a lone hit and its backing.
3. Write the synth as a model file of named, bounded parameters — `AI/Python/Audio/hex_lock_layers.py` is one.
4. Run `fit`, then `compare`, and read its picture; the model gets a layer where the offsets cluster.
5. Write the layers for Audacity and play them to the user.

- A hit in a trailer or sound-pack video sits on music and on the tails of earlier hits; that backing, not the
  hit, often owns everything below ~150 Hz.
- Aligned hits that correlate in every band from the onset on are one sample; correlation before the onset is
  a shared backing instead.
- Two copies of one sample give the clean target: their cross-spectrum keeps what they share and cancels their
  unrelated backings.
- A lone hit is measured as its power above the backing's average just before it; for a hit that swells in, that
  window ends before the swell starts.
- A hit's rise in dB over its backing depends on that backing; subtracting the backing's power in linear terms
  gives the hit's own spectrum and decay, comparable across recordings.
- A section of many events is judged by playing the synth in the same scene — the same rate over its own bed — and
  comparing both with `uv run AI/Python/Audio/compare_sections.py`: `events` for the anatomy with the backing
  subtracted, `octaves` for the section's balance, `picture` for spectrograms stacked on one scale.
- A reference's backing moves under its events below ~250 Hz; only the bands above it judge the events.
- A lone hit's low bands still hold backing the window missed: `target` takes a frequency below which the synth
  is only kept under the measurement, never pulled up to it.
- A model is layers of band-split noise, each band under a parametric envelope: few parameters, each one a
  sound-design control — a level, a tilt, an attack, a decay rate, a strike time.
- The individual clicks of an attack show in the energy above 3 kHz every half millisecond; each becomes a strike
  time in the model.
- The fit runs least squares on the decibel mismatch from several random starts, since one start stops in a
  local minimum.
- The fit is done when synth-to-target sits near synth-to-itself on another noise seed: the rest is measurement
  noise.
- A parameter pinned at its bound means its layer is doing another layer's job, or a layer is missing.
- A video's audio stops near 16 kHz, the codec's ceiling; the synth rolls off there too to match it.
- A real hit's highs are wide and its lows near mono: the synth's noise carries the reference's left-right
  coherence per band.

## Reading the picture

- Top: the waveform with its envelope — attack, decay, level, clicks.
- Middle: a log-frequency spectrogram, dark to bright; the cyan line is the centroid.
- Bottom: the average spectrum in decibels.
- Evenly spaced horizontal lines are harmonics; unevenly spaced lines are inharmonic, metallic or bell-like.
- Lines below the fundamental or bent back down at the top are aliases.
- A vertical smear is a transient; a haze filling the height is noise or aliasing.
- A slanted line is a pitch glide; a line fading out is that partial's decay.

## CLAP ear

A local CLAP model would score each exported WAV against text prompts; it needs PyTorch and a checkpoint, about
1–2 GB, and is not installed. It ranks variants of one sound by how well they match a description, not by
quality: CLAP rewards semantic fit, never realism or polish.

## Sources

- Cherep, Singh, Shand — Creative Text-to-Audio Generation via Synthesizer Programming (CTAG), ICML 2024 — arxiv.org/abs/2406.00294
- Chu et al. — Text2FX: Harnessing CLAP Embeddings for Text-Guided Audio Effects — arxiv.org/abs/2409.18847
- Doh et al. — Can Large Language Models Predict Audio Effects Parameters from Natural Language? (LLM2Fx) — arxiv.org/abs/2505.20770
- Engel et al. — DDSP: Differentiable Digital Signal Processing — github.com/magenta/ddsp
- Wu et al. — LAION-CLAP — github.com/LAION-AI/CLAP
- Gong et al. — AST: Audio Spectrogram Transformer — arxiv.org/abs/2104.01778
- Evans et al. — Stable Audio Open — arxiv.org/abs/2407.14358
- Hung et al. — TangoFlux, ICLR 2026 — arxiv.org/abs/2412.21037
- Kyutai — Neural audio codecs: how to get audio into LLMs — kyutai.org/codec-explainer
- Tailleur et al. — Correlation of Fréchet Audio Distance with human perception is embedding dependent — arxiv.org/abs/2403.17508
