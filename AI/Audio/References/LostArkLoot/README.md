# Lost Ark — loot from a chest (gameplay)

Source: youtube.com/watch?v=CWy_WFlYCkQ, 17 s of gameplay (FINAL CBT), not a sound pack: every sound sits on the forest
ambience (~ −33 dBFS) and on each other, so `cuts.json` holds silence fragments and `labels.txt` the hand cuts. Reference only.

| Cut | Span s | What |
|---|---|---|
| 000_ChestPop | 6.45–6.95 | Chest bursts open: noise burst, low thump ~190 ms, G5 (784 Hz) saw chime late |
| 001_ChestLift | 7.30–7.75 | Noise swell 400 ms with 590 / 1480 Hz tones into a hit |
| 002_LootBurst | 7.72–9.80 | Items spraying out, 37 events, loudest bed |
| 003–006_Pickup1–4 | 9.8–15.4 | One item each (x23, x12, potion, ...): a shared inharmonic glint (1315 1690 2690 4038 4790 6540 7780 9110 Hz) over an item-specific thump / sweep; Pickup1 and 3 end on 2.6 kHz ticks, Pickup4 on a ~25 Hz sparkle rattle |
| 007_EndCue | 16.2–17.1 | Low-level tail cue |

## Reproduced (`AI/Python/Audio/Patches/LostArkLoot/`)
| Patch | Distance / own noise floor, dB |
|---|---|
| ChestPop | 6.9 / 5.1 |
| Pickup2 | 10.9 / 4.7 — bed, clicks, glissando, glint partials still miss the dense broadband bed and the 8–16 kHz sparkle |
| Pickup1, 3, 4 | written from the anatomy, not fitted |
| ChestLift, LootBurst, EndCue | not written |
