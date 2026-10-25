# TutorialTeleporters — pending

Goal: tutorial rooms are sealed by fixed walls and spaced apart; the player moves room to room through the lobby's `AGeoTeleporter`, which opens when the room is done and closes behind them, so they get used to the pad.
Docs written: `Actor/Buttons/CLAUDE.md` ("Open / closed"), `Encounter/Action/CLAUDE.md` (`GeoSetTeleporterOpenAction`), `AI/Tutorial/Flow.md` ("Rooms"), `AI/Python/Level/CLAUDE.md`.
Done 2026-10-21: code, `GeoTrinity.Encounter.SetTeleporterOpen`, map and encounter scripts applied (no `Tut_*_Door` / `_Barrier` actor left), `pie_tutorial_flow.py` passes (pad opens, entry opens on arrival, both seal behind the player).

## Left (user)
- [ ] The pad Blueprint's open / close animation on `OnOpenChanged` (in progress); all tutorial pads use `/Game/Actors/TriggersActors/BP_Teleport`
- [ ] Open question: `ApplyOpen` hides the pad in C++ the instant it closes, which would cut the close animation; if the animation needs the mesh until it ends, hide on an animation-finished call instead

## Status 2026-10-25
`GeoSetTeleporterOpenAction`, test and `OnOpenChanged` exist. Remaining: `BP_Teleport` open/close animation (editor).
