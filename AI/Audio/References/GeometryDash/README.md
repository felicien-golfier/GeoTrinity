# Geometry Dash — all sound effects

Source: youtube.com/watch?v=TpjwJCjmBmk, 32 s, each sound named on screen; `cuts/NN_<Name>.wav` (the `_Tail` cuts
are the ends of the sound before them). A geometric game's reward and death sounds: the closest style to
GeoTrinity's shapes among the references. Reference only, never shipped.

## Reproduced (`AI/Python/Audio/Patches/GeometryDash/`)
| Sound | Distance / its own floor |
|---|---|
| Death | 4.5 / 2.7 dB |
| Coin | 6.3 / 0.1 dB: the strikes fit; the synth stays 13–30 dB short under 250 Hz after them, where the level's music plays under the coin |

## Heard (`survey.txt`)
| Sound | Anatomy |
|---|---|
| Coin | A two-strike metal ping: a 1.52 kHz strike with a 6.3 kHz partial, then ~55 ms later the ringing set 1.01, 2.03, 2.71, 4.16 kHz whose 2.03 and 2.71 kHz lines hang ~1.5 s, over a tiny 87 Hz thud. |
| Death | Noise, loudest at 63–250 Hz and peaking at 120 ms, the mids 14–17 dB down, crackling at ~22 Hz, 40 dB down a second later; mono. |
| Level Quit | One clean tone at 442 Hz, harmonics falling 16 dB per octave, 0.75 s. |
| Level Play | A 921 Hz line held 115 ms in a 0.84 s sound, 94 % tonal. |
| Achievement Unlock | A tonal swell (97 %) on 524 Hz with every harmonic, 1.2 s. |
| Shards, Diamonds, Orbs | Sparkle: high lines at 13 kHz over mid tones round 1.5 kHz, 0.5–1 s. |
