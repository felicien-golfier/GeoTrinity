# Geometry Wars: Retro Evolved — sound effects

Source: youtube.com/watch?v=dneyDGseYMY, 68 s, the game's effects one after another with no names; `survey.txt`
measures every cut. The neon-geometry shooter closest to GeoTrinity's look. Reference only, never shipped.

## Reproduced (`AI/Python/Audio/Patches/GeometryWars/`)
| Sound | Distance / its own floor |
|---|---|
| Tone_27 | 3.9 / 0.2 dB |
| Tone_35 | 8.1 / 1.0 dB: the square's filter dome is right, the balance of the low voice under it is not |
| Wobble_43 | 6.8 / 0.2 dB |

## Heard
- Spawns and enemies are held synth tones on a harmonic series, each a pitch of its own: 27 is ~330 Hz with its
  2nd harmonic nearly as loud (a narrow pulse's colour), flat from a 34 ms attack to a hard cut; 35 is two voices,
  a low tone falling 338 → 255 Hz and a square on 298 Hz whose lowpass opens to 10 kHz and closes again; 43 is a
  saw swooping 158 → 497 Hz and back down while pulsing about 40 times a second.
- Shots and hits are short noise bursts (12, 13: 0.2 s, broadband); the bomb is 4 s of broadband noise with its
  low end loudest, dying slowly (17, 33).
