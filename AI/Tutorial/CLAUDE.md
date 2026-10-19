# AI/Tutorial

Design docs of the tutorial: tone, lore, the Cat, the board of pages. Design only; in-game code folders get their own `CLAUDE.md` when the implementation is written (see `AI/Pending/Tutorial.md`).

| File | Role |
|---|---|
| `Tone.md` | Voice: MMO parody, sarcasm, joke targets, limits. Read before writing any line of text. |
| `Lore.md` | Premise, cast, world rules, why the player is stuck, how it ends. |
| `Cat.md` | The Cat: role, personality, shape language, voice, reusable line bank. |
| `IntroBoard.md` | Pages 1-3: game night, absorption, landing. Layers, motion, beats. |
| `Steps.md` | The teaching steps after landing (skeleton; content arrives later). |

## Runtime
Each step of `Steps.md` is a step of an `AGeoEncounter` (`Source/GeoTrinity/Public/Encounter/CLAUDE.md`): "Done when" = a rule's trigger + condition with `bCompletesStep`; "On fail" = another rule on the same step. No tutorial-specific director.

## Artifacts (working surface)
Two Claude.ai artifacts, editable by the user, listed in `AI/Artifacts.md`:
- **Tutorial Lore** — shareable read-through of `Lore.md` / `Cat.md` / `Tone.md`.
- **Tutorial Board** — one page per important step, drawn and with its text.

While drafting, the **artifact holds the exact on-screen text** (captions, dialogue); these docs hold structure, rules and intent. When a page is validated, its text moves to the in-game source named in `Steps.md`; the artifact then only illustrates it.

## Invariants
- One speaker: the Cat, source of all sarcasm. No other character talks. It reads as a fellow prisoner who helps, and knows more than it tells (`Cat.md`).
- Every page has an on-screen purpose (set up, turn, or teach); a page that only jokes is cut.
- Teaching never waits on a joke: the player can act while the Cat talks; skippable dialogue.
- Shapes follow `AI/ArtDirection.md` (flat neon geometry); the real-world scenes (pages 1-2) are the only place a non-geometric ground is allowed, and even there figures are built from the same solids.
