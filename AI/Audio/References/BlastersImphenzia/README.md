# Imphenzia — Blasters, Game-Ready Sound Pack (demo 1)

Source: youtube.com/watch?v=-PP-FaECD-Q, 177 s. A Unity turret firing one file at a time at a set rate of fire;
the file name and the rate show at the bottom left. `labels.txt` holds the cleaned names and spans, each span a run
of shots of one file: `<Length>_<Loudness>_<Pitch>_<NNN>[_Stereo]_RoF<shots a minute>`. Two barrels fire in turn,
so a RoF60 run fires every ~0.5 s; a RoF60 or RoF120 run is where single shots are cut from. The demo scene adds a
low hum under the shots. Reference only, never shipped.

## Reproduced (`AI/Python/Audio/Patches/BlastersImphenzia/`)
| Sound | Anatomy |
|---|---|
| VeryLong_VeryLoud_HighMid_001 (one shot, 1.5–2.24 s of its run) | A broadband crack over 800 Hz, 180 ms; a saw pew 1.5 kHz → 360 Hz in 140 ms, dead in 75 ms; a sine boom 180 → 60 Hz in 50 ms held 0.66 s; a noise body round 330 Hz ringing 0.7 s; a 6.8 kHz whine 30 dB down. |

## Heard
- Every blaster opens on a broadband crack to 16–20 kHz and carries descending pew lines from 2–10 kHz.
- Length names the tail: Short ~0.2 s, Medium ~0.4 s, Long ~0.7 s, VeryLong past 1 s.
- Muffled ones are low-passed near 1–2 kHz with a noisy low burst; Precharge ones swell for ~0.2 s before the shot.
