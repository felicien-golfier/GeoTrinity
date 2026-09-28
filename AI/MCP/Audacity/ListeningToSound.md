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
- Fetch a video's audio with `uvx yt-dlp -x --audio-format wav --ffmpeg-location <ffmpeg>`; without a system
  ffmpeg, the `imageio-ffmpeg` package ships one, found with its `get_ffmpeg_exe()`.
- References go to `Music\SFX\Reference\`, cut to the passage the user named.
- A montage of several sounds is cut apart with `uv run AI/Python/Audio/split_on_silence.py <file.wav>`, then
  each cut is measured on its own; a whole-montage printout averages unrelated sounds into nonsense.
- A sound-pack video usually names each sound on screen: fetch the video at low resolution and tile one frame
  every ~0.7 s into a single image with ffmpeg's `fps` and `tile` filters; the label leads its sound slightly.
- The script's 1024-point spectrogram has ~47 Hz bins, too coarse below 200 Hz: pitch a sub with an 8192- or
  16384-point transform.
- Stereo is measured as mid against side and the correlation of left and right: a reverberant tail sits near
  0.7, a dry synth layer at 1.0.
- A file whose channels are in anti-phase folds to near silence in mono; `listen.py` then measures left minus
  right and says so.

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
