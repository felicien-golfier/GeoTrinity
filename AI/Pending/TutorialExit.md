# TutorialExit — pending

Goal: when the tutorial has no next step, the final exit takes the player to the Draft map (Training Room) instead of a pad into an unbuilt room.
Priority: 2
Docs written: `AI/Tutorial/Flow.md` ("Rooms" final exit pad, "Exit"), `AI/Python/Tutorial/CLAUDE.md` (`tutorial_encounters.py` row).
Done 2026-10-21: script + map applied; `Tutorial/Runtime/pie_tutorial_jump.py` (`#Dummies` start): the exit pad opens when the room is cleared, stepping on `Tut_Dummies_PadExitVolume` travels to `DraftMap` at the `TrainingRoom` start (-6000, 0), no teleport to the Zones room.

## Left (user)
- [ ] Open question: the exit pad's look is the normal portal; fine, or a distinct colour for "last door"?
