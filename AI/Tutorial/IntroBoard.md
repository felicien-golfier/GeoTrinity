# Intro Board — pages 1 to 3

Three full-screen illustrated pages before gameplay. Text of each page lives in the Tutorial Board artifact (`AI/Artifacts.md`); this file holds structure and intent.

## Depth model ("animation without animation")
Every page is 3 layers drifting at different speeds on a horizontal axis. No frame animation: only translation, constant speed (`AI/ArtDirection.md`, rule 5).

| Layer | Content | Drift |
|---|---|---|
| Back | Room: wall, window, posters, shelf | slowest |
| Middle | Table, figurines, dice, snacks, players' hands | slow |
| Front | The Cat sliding across, large, partly out of focus | fastest, crosses the frame once |

## Page 1 — Game Night
- Purpose: set up the table, the 3 figurines, the Cat.
- Beat: the figurines (Square, Circle, Triangle) stand on a map with a hexagon boss at the far end; the Cat narrates in captions (players are silent); its own head and tail slide through the front layer while it pretends it is not there.
- Exit: advance on input / after the caption finishes.

## Page 2 — The Push
- Purpose: the turn. Absorption, Jumanji-style.
- Beat: same table, light bleeding out of the board as hexagons; the three figurines glow, the players' silhouettes stretch toward the board and shrink into their shapes; the Cat, in the back layer now, paw near the toppled glowing piece (a stumble or a push, both readable), hexagon eyes lit, itself dragged in last.
- Exit: white-out into the landing.

## Page 3 — Landing
- Purpose: first in-game screen. New geometric place (an empty arena-lobby on the lattice), one shape: the Circle, the only class of the tutorial (the other classes are discovered in the Training Room, `Steps.md` step 13). The Cat's hologram speaks in a dialogue box.
- Beat: the Cat says it is stuck too and will get them out, insults them, hints it knows more than it says, jokes that the Healer is probably the only time the player will ever play one (forced, it is the tutorial), announces the first lesson. Its text ends on a doable action (move) and hands over control.
- Situation line, right after the Cat's first quip: it explains where they are. The game is *Dungeon & Dihedral*; the Cat and the Healer landed here by luck, the two other friends surely landed in front of a boss and are already dead. Text lives on Board page 3.
- Hand-off: its last line leads into `Steps.md` step 1 (Move).

## Implementation notes (target, once the user greenlights step 4)
- Pages 1-2: 2D UI screens in `GeoTrinityUI` (layered images, scripted drift), shown as the tutorial map's loading screen (a per-map kind chosen in `SGeoLoadingScreen::Make`, `GeoTrinityUI/Public/LoadingScreen/CLAUDE.md`); page 3: a real level with a UI dialogue box.
- Layer images and drift speeds are data, not hard-coded (tweakable in Blueprint per `AI/UI.md`).
