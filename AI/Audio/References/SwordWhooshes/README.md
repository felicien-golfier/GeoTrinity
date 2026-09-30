# SwordWhooshes

Source: youtube.com/watch?v=o7nAIbtMoxQ. 2MirrorsDialogue — Sword Whoosh Sound Effects, 25 s, five sword swings
over a still of a sword, one per cut. Reference only, never shipped.

## Reproduced (`AI/Python/Audio/Patches/SwordWhooshes/`)
| Sound | Anatomy |
|---|---|
| Swish_01 | Noise tilted -1.2 dB/octave under a lowpass 2.7 → 3.3 kHz at the peak → 1.1 kHz on the way out; the level jumps to -19 dB in 20 ms, swells straight in dB to its peak over 190 ms, falls 15 dB in 230 ms and fades out over 330 ms more. A band gliding 2.5 kHz → 0.9 kHz sits 19 dB under it: the air's whistle hardly counts. |

## Heard
- All five are the same gesture: 410–600 ms, a swell of 90–170 ms to the peak, down 20 dB 140–250 ms after it,
  0 % tone. The spectrum is broad: 63–250 Hz stand within 2 dB of the loudest band, 8 kHz 12–14 dB under it. The
  centroid starts at 600–940 Hz and ends 530–730 Hz, darker as the blade leaves.
- Against them, the synthesised `SFX_Ninja_Swish_*` (2026-09-29) are about half as long (190–220 ms, a 60–80 ms
  swell), a band around 2 kHz with nothing under 500 Hz; `SFX_Ninja_Whoosh_Mid` has a real swish's timing (156 ms
  swell, 174 ms fall) but the same missing low end. An air cut may want that thinness; a blade's body does not.
