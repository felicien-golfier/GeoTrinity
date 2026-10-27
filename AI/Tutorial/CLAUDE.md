# AI/Tutorial

Design docs of the tutorial: tone, lore, the Cat, the board of pages. Design only; in-game code folders get their own `CLAUDE.md` when the implementation is written (see `AI/Pending/Tutorial.md`).

| File | Role |
|---|---|
| `Tone.md` | Voice: MMO parody, sarcasm, joke targets, limits. Read before writing any line of text. |
| `Lore.md` | Premise, cast, world rules, why the player is stuck, how it ends. |
| `Cat.md` | The Cat: role, personality, shape language, voice, reusable line bank. |
| `CatAnimation.md` | The Cat's rig, idle / walk / portal-jump clips, how to animate a cat for the top camera. |
| `Data/CatLines.txt` | Hand-edited bank of the Cat's random lines (tips, puns, nags) by `[Category]`; read live by the game and the editor picker (`Source/GeoTrinity/Public/Tool/CatLine/CLAUDE.md`). Replaces `LoadingTips.md`. |
| `IntroBoard.md` | Pages 1-3: game night, absorption, landing. Layers, motion, beats. |
| `Steps.md` | The teaching steps after landing (skeleton; content arrives later). |
| `Flow.md` | Maps, entry (menu button, first-launch Cat prompt), jump to a room (dev), skip, exit to the Training Room, replay and loot. |

## Runtime
Each step of `Steps.md` is a step of an `AGeoEncounter` (`Source/GeoTrinity/Public/Encounter/CLAUDE.md`), one encounter per room or step group: "Done when" = a rule's trigger + condition with `bCompletesStep`; "On fail" = another rule on the same step; what one rule needs from another (falls before the reach, colors visited, two goals met) goes through step counters. No tutorial-specific director.
What a room holds is written on its encounter, not as assets: enemies = `BP_Dummy` + an inline setup on the spawn action, plain zones = `GeoSpawnZoneAction` (`Encounter/Action/CLAUDE.md`). A new Blueprint or ability only for real new behaviour (miniboss, summoned dummy, wave patterns).

## Artifacts (working surface)
Two Claude.ai artifacts, editable by the user, listed in `AI/Artifacts.md`:
- **Tutorial Lore** — shareable read-through of `Lore.md` / `Cat.md` / `Tone.md`.
- **Tutorial Board** — one page per important step, drawn and with its text.

While drafting, the **artifact holds the exact on-screen text** (captions, dialogue); these docs hold structure, rules and intent. When a page is validated, its text moves to the in-game source named in `Steps.md`; the artifact then only illustrates it.

## Invariants
- One speaker: the Cat, source of all sarcasm. No other character talks. It reads as a fellow prisoner who helps, and knows more than it tells (`Cat.md`).
- Every page has an on-screen purpose (set up, turn, or teach); a page that only jokes is cut.
- Teaching never waits on a joke: the player can act while the Cat talks; skippable dialogue: text types out, F shows it whole, F again ends the line and the encounter moves on (mashing F = fastest run). One exception: the Landing room (steps 0-1) locks movement until the Cat says the WASD line, since nothing can be acted on before (`Steps.md`, step 1).
- Shapes follow `AI/ArtDirection.md` (flat neon geometry); the real-world scenes (pages 1-2) are the only place a non-geometric ground is allowed, and even there figures are built from the same solids.
