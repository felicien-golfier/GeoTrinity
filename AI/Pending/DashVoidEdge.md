# DashVoidEdge — pending (question)

Goal: decide whether the Dash room's void kills a player whose centre is over the hole, like the hex boss's fall rule, instead of only when the capsule leaves the floor.
Priority: 2
Docs written: none (a question for the user; nothing changed).

## Found (2026-10-28 playtest)
- The player landed with the centre over the void, the capsule's edge still on the floor, and stayed alive: the Dash void is real geometry (`tutorial_map.py`, `VOID_WIDTH` 300) killed by the world `KillZ` (`Characters/CLAUDE.md`, "Void"), so the engine's floor check (capsule perch) decides.
- The hex boss has a stricter rule: `AGeoHexArena::IsSupported`, centre over an alive tile or within `FallGraceMargin` (30 cm) of an alive neighbour's border (`Actor/Arena/CLAUDE.md`, "GeoHexArena").
- `AI/Tutorial/Steps.md` step 2 says the hex tile rule is not used on purpose (a real hole, no fall system of its own).

## Questions
- [ ] Make the void an arena hole done in code (a fall rule like `IsSupported` over a rectangle, or a hex arena with a void line), so landing near the edge follows the same limit as the hex boss?
- [ ] Or keep real geometry and tune the character's perch (`PerchRadiusThreshold` / `PerchAdditionalHeight` on the movement component) to the same 30 cm margin, for every hole of the game?
- [ ] Bot: `Path.DashFalls` / `FullRun` dash landing would get a "lands on the edge: dies" case.
