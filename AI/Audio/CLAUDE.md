# AI/Audio — sound workspace

Everything made or gathered for sound that does not ship. Shipped sources → `SourceArt/Audio/`, recipes → `AI/Python/Audio/Patches/`.

| Folder | Holds | Written by |
|---|---|---|
| `References/<Pack>/` | Pack audio, video, `cuts/`, `labels.txt`, `survey.txt`, `README.md` | `fetch_reference.py` |
| `References/*.wav` | Single references cut to the passage asked | by hand |
| `Reproductions/<patch>/` | Synth, reference-then-synth A/B, picture | `fit_patch.py` |
| `Drafts/<Sound>/` | Drafts to hear, new name per iteration, with `.aup3` | kit and score scripts |
| `BoneMotion/` | Montage bone motion a score is timed from | `AI/Python/Boss/Anim/dump_bone_motion.py` |

Audio, video, pictures and `.aup3` are git-ignored; READMEs, surveys, labels, `cuts.json` are checked in.

## Reference reproduction campaign (since 2026-09-29)
Goal: reproduce commercial SFX from sound-pack preview videos with fitted patches to get better at SFX; every rule learned goes into `AI/MCP/Audacity/GameSoundDesign.md` / `ListeningToSound.md` the same session. References are measured, never shipped.

- Pipeline (`uv run` in `AI/Python/Audio/`): `fetch_reference.py` → `dissect.py` (`survey`) → `sfx_patch.py` → `fit_patch.py` (A/B WAV and picture in `Reproductions/`). The synth is trimmed as deep under its peak as the reference's cut; fits before 2026-09-29 22:00 used its own "bed" and cut slow swells (refit may shift slightly).
- Packs in `References/` (README + survey each): UIImphenzia, BlastersImphenzia, SciFiWeaponsGHA, BuffHeal, SciFiBombImpacts, MagicSpells, PowerUp, UIFuturistic, ForceFields, RetroGame, SciFiWeaponsPrinciple, GeometryDash, GeometryWars, SwordWhooshes (5 clean swishes: the whoosh reference), Swooshes. Whooshes and GameWhooshes are music: unusable.
- Scores (distance / own noise floor, dB): beep 0.9/1.0, click 2.5/1.9, pickup 4.0/1.2, zap 4.3/0.0, chip arpeggio 4.9/3.3, power-up 5.2/0.8, impact 5.1/2.6, swish 5.3/4.0, laser 5.4/2.6, stat up 5.8/3.9, jump 6.1/0.2, coin 6.3/0.1, blaster 7.0/4.5, Bomb_13 5.3/3.1. Not reproducible as written: the Imphenzia triple alert.
- Unfinished: Heal (fifth strike + room; its JSON holds starting values), Shield_84, Impact_00 (keeps its earlier fit).
- Measured: the ninja kit's swishes are half a real swish's length and lack its low end (SwordWhooshes README).
- Next: rebuild the playlist (`fit_patch.py playlist`), add Bomb / Heal / Shield rows to GameSoundDesign.md's reproduced table, user listens, then turn the best recipes into GeoTrinity sounds by class (Circle sine, Square square, Triangle triangle / saw).
- Another session may edit the same sound docs (star boss): re-read before editing.
