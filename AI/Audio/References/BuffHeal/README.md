# TechieMay — Buff, Debuff and Heal SFX (preview)

Source: youtube.com/watch?v=8RP97zcYgVg, 27 s, each sound named on screen: cuts 00 Stat Up, 02 Stat Down,
03 Super Stat Up, 04 Super Stat Down, 05 Minor Heal, 07 Heal, 10 Major Heal (the others are tails). Reference only.

## Reproduced (`AI/Python/Audio/Patches/BuffHeal/`)
| Sound | Anatomy |
|---|---|
| Heal (07) | Bell voices climbing over 0.7 s — A4, F5, G5, A5, then B♭5, F6, C♯6, G♯6 — partials 1× (60 dB in 3.2 s), 3.1× (−12 dB, 1.5 s), 5.7× (−4 dB, 0.8 s), 1.4 ms strike; the fundamentals hang as a chord. |
| Stat Up (00) | A sub thump under 175 Hz, 0.3 s; a noise band rising 670 Hz → 2.2 kHz over 0.46 s with a 9 Hz flutter, cut dead at 0.6 s into a 0.9 s room; very wide stereo (correlation 0.3). |

## Heard
- Minor Heal and Heal share the same bell arpeggio; Major Heal swells in over ~0.4 s and adds a longer shimmer.
- Stat Down and Super Stat Down are the ups' noise gestures falling instead of rising, darker at the end.
