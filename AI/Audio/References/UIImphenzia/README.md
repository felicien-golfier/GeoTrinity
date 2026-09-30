# Imphenzia — Game-Ready User Interfaces (preview)

Source: youtube.com/watch?v=Tyg702D_02E, 377 s. A demo playing random files from ten categories (Beep, Bells, Chip,
Click, Mechanical, Noise, SciFi, Synth, Whoosh, Zap) over a quiet music bed ~30 dB under the sounds. Each file's
name shows under the category buttons; `labels.txt` holds the OCR'd spans, `cuts/NNN_<name>.wav` the sounds.
Past ~330 s the demo fires names faster than their sounds: those spans are unreliable. Reference only, never shipped.

The names carry the recipe: `Beep_<Wave>_<Note>_<Hz>_<ms>`, `Chip_<One|Two>_<N>_Note_<Up|Down>`, `Wet` = with a
stereo reverb, `Dry_Mono` = none.

## Reproduced (`AI/Python/Audio/Patches/UIImphenzia/`)
| Sound | Anatomy |
|---|---|
| Beep_Sine_Square_A2_110Hz_125ms | A sine with a square 3.5 dB under it on the same 110 Hz, the square low-passed at 400 Hz: the odd harmonics fade out past the fifth; 2 ms attack, 120 ms held, 50 ms release. |
| Zap_29 | One pure sine: 11 kHz → 350 Hz in 3.5 ms, then up exponentially to 16 kHz in 42 ms, even level, 8 ms cut. |
| Jump_01 | A square wave 480 → 1400 Hz exponentially over 170 ms, falling 23 dB, cut at 150 ms. |
| Click_33 | 40 ms: a triangle chirping 450 → 1800 Hz in 16 ms, low-passed at 1.6 kHz, over a dull noise tick 8 dB lower. |
| Chip_One_Three_Note_Up_05 | A near-square pulse stepping every 31 ms through its notes and their lower octaves, landing on 357 Hz, into a 1.2 s room. |

SciFi_Notification_Alert_Triple_27 is not reproduced: its chatter is the same figure in each of its three bursts,
which random blips cannot fit (15 dB away, 10 dB from themselves); its notes have to be written out.

## Heard, not yet reproduced
- Zaps are sine or noise chirps under 70 ms; most sweep up (Zap_14: 165 → 2650 Hz in 28 ms).
- Jumps rise 1.5–2.5× in 100–150 ms on a square or pulse wave.
- Clicks are 5–70 ms: a noise tick or a short tone, "Dark" low-passed, "Bright" not.
- Wet variants carry a stereo room of ~0.5–1 s about 15–20 dB under the dry sound.
