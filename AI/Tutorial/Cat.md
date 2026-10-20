# The Cat

Mascot, narrator, the **only speaking character**. It incarnates the sarcasm: every joke in the tutorial is the Cat's. Players and bosses never talk.

## Role
- Intro: narrates pages 1-2 in voice-over captions while sliding through the foreground of page 1 (the "camera move without animation").
- Tutorial: the only voice, as a dialogue portrait and, in the landing hub, a geometric hologram.
- Later: commentator on deaths, loot and wipes; guide toward the exit.

## Two readings (the central trick)
| Reading | What the player believes at first | Truth, revealed in pieces |
|---|---|---|
| **Surface** | A fellow prisoner of the world, sarcastic coping mechanism, wants out as much as you and helps you get there. | |
| **Under** | | It pushed them in, got locked in by mistake, and is the final boss. It knows how the world works, who the bosses are, what the exit costs. It tells only what the current lesson needs, deflects the rest with a joke. |

Rules of the double reading:
- Early on, everything the Cat does must be readable as help; the clues only make sense afterwards (e.g. page 2's paw on the glowing piece looks like a stumble).
- It never lies outright; it omits, jokes past the question, answers a different one.
- A question the player could ask ("how do you know that?") gets a joke and a half-answer, never a full one.
- Hints are small and recurring (a pause, a line that says "again" instead of "first time", a boss it names before meeting it). What is revealed and when is decided in `Lore.md` ("Secrets ladder").

## Personality
Smug, bored, theatrical, helpful through insult. Treats the players as customers on an onboarding funnel while visibly caring that they survive. Never insulting without a useful fact attached, never useful without an insult attached.

## Shape language (`AI/ArtDirection.md`)
| Part | Shape |
|---|---|
| Head | Circle |
| Ears | 2 triangles |
| Eyes | 2 hexagons, the boss shape: foreshadows that the Cat is tied to the bosses |
| Body | Elongated rounded rectangle / stacked polygons |
| Tail | Chain of segments, the part that animates |
| Colour | The bosses' colour and variations of it (tints / shades), outside the class colours (blue / yellow / green): the Cat is tied to the bosses. Four roles: base, shade, ink (darkest), light (eyes / nose). |

In the real-world pages (1-2) the Cat is a soft silhouette with a halo; in game it is full neon geometry. Same proportions.

## In game
- Dialogue box: the Board's face portrait beside the line (`HUD/Tutorial/CLAUDE.md`, "Cat face"), identical shape and motion; mouth moves while it speaks.
- The 3D Cat (`AGeoCat`, `Actor/CLAUDE.md`) lives in the Landing room, same palette. How it moves: `CatAnimation.md`; staged by the encounter (`Steps.md`, step 1 "Cat staging").

## Voice samples (draft bank; final text lives in the Tutorial Board artifact)
- "Welcome, adventurers. Please keep your hands inside the game at all times. I've lost mine already."
- "Heal. You're the Circle. No, nobody will thank you. Yes, it is on purpose."
- "Tutorial complete. Now do it again with feeling."
- "Defeat is temporary. My disappointment is permanent."
- "Press W to walk. Congratulations, you have unlocked walking."
- "How do I know that? Lucky guess. Next question."

## Rules
- Never apologises for the situation, but never claims to have caused it.
- Never gives a lesson without a doable action in the same breath.
- Does not speak during a boss's lethal attack; commentary fits the safe windows.
