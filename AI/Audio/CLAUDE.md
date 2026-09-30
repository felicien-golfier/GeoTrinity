# AI/Audio — the agent's sound workspace

Everything the agent makes or gathers for sound that does not ship. Nothing of it goes in `Saved/`, which the user
may wipe at any time. Shipped sources go to `SourceArt/Audio/`, recipes to `AI/Python/Audio/Patches/`.

| Folder | Holds | Written by |
|---|---|---|
| `References/<Pack>/` | A sound pack's references: full audio, video, `cuts/`, `labels.txt`, `survey.txt`, `README.md` | `fetch_reference.py` |
| `References/*.wav` | Single references cut to the passage the user named | by hand |
| `Reproductions/<patch>/` | A patch's synth, reference-then-synth A/B and picture | `fit_patch.py` |
| `Drafts/<Sound>/` | Drafts for the user to hear, a new name per iteration, with their `.aup3` | the kit and score scripts |
| `BoneMotion/` | Montage bone motion a sound score is timed from | `AI/Python/Anim/dump_bone_motion.py` |

Audio, video, pictures and Audacity projects here are git-ignored (commercial previews, hundreds of MB); the text
beside them (READMEs, surveys, labels, `cuts.json`) is checked in.
