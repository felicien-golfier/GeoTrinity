# TutorialTeleporters — pending

Goal: tutorial rooms are sealed by fixed walls and spaced apart; the player moves room to room through the lobby's `AGeoTeleporter`, which opens when the room is done and closes behind them, so they get used to the pad.
Docs written: `Actor/Buttons/CLAUDE.md` ("Open / closed"), `Encounter/Action/CLAUDE.md` (`GeoSetTeleporterOpenAction`), `AI/Tutorial/Flow.md` ("Rooms"), `AI/Python/Level/CLAUDE.md`.

## State (written, not built: the user's editor is open with Live Coding, so `Tools\Build_Editor.bat` refused)
- Next: editor closed → `Tools\Build_Editor.bat`, `Tools\Run_Tests.bat GeoTrinity.Encounter`, then run `tutorial_map.py` and `tutorial_encounters.py` headless (map script now always rebuilds: relocates rooms for `ROOM_GAP`, removes doors / barriers, adds walls, pads, arrival zones); check `actor.rename` gave the pads their sort names (`Tut_Pad_NN_Exit` / `_Entry`) and that `/Game/Actors/TriggersActors/BP_Teleport` takes `bOpen`
- Added beyond the plan: tag `Teleport.Tutorial`; each encounter gets trailing steps `Leave` (player enters the next arena volume → start next encounter, open its entry pad) and `Seal` (player enters the next arrival zone → close entry and previous exit); exit pad labels `Tut_<Room>_PadExit` / `_PadEntry`

## Code (to implement)
- [x] `Actor/Buttons/GeoTeleporter.h/.cpp` — `bOpen` (`EditAnywhere`, default true, `ReplicatedUsing = OnRep_Open`, `GetLifetimeReplicatedProps`, `bReplicates = true`); `SetOpen(bool)` (server); `ApplyOpen` (hidden in game when closed, mesh overlap off when closed, `PendingArrivals.Empty()`); `BlueprintImplementableEvent OnOpenChanged(bool)`; `BeginPlay` applies the start state; the overlap handler returns while closed
- [x] `Encounter/Action/GeoSetTeleporterOpenAction.h/.cpp` — `Teleporter` (`TObjectPtr<AGeoTeleporter>`), `bOpen`; copy of `GeoSetBarrierClosedAction`
- [x] Tests (`GeoEncounterTests.cpp` / a teleporter test, `AI/CodingStyle.md` "Tests"): `GeoTrinity.Encounter.SetTeleporterOpen` — action flips the pad, a closed pad does not move a player, reopened it does, `PendingArrivals` cleared on close

## Scripts
- [x] `AI/Python/Level/tutorial_map.py` — barrier/door code gone (`build_barrier`, `DOOR_*`, `BARRIER_BP`) → fixed wall closing each room's +X end (same wall kit as the side walls); `ROOM_GAP` (placeholder 1500 cm: check the camera frame shows no neighbour room) added to the room pitch (`room_x`); per room `Tut_Pad_<NN>_Exit` (rooms 1-8) and `Tut_Pad_<NN>_Entry` (rooms 2-9) `AGeoTeleporter`s, one `TeleportTag` (`Teleport.Tutorial`, create the tag), `bOpen` false; exit at a fixed spot near the room's +X end, entry at the room's `TargetPoint.Entrance`; an arrival-zone volume past each entry
- [x] `AI/Python/Level/tutorial_encounters.py` — `door()` → `pad(label, open)` (`GeoSetTeleporterOpenAction`); `hand_off`: delay (last Cat line duration, `LINE_SECONDS` min) → open exit pad; start the next encounter as now; the next encounter's first `OnEnter` opens its entry pad; a rule `entered(arrival volume)` closes the entry pad and the previous exit pad; drop `close_own_door`; `organize_folders` category `Doors` → `Teleporters`

## Editor / Blueprint (by hand or script)
- [ ] The pad Blueprint's open / close animation on `OnOpenChanged` (user, in progress); all tutorial pads use that one Blueprint
- [ ] Delete the `Tut_*_Door` / `Tut_*_Barrier` actors from `/Game/Maps/Tutorial` (the map script is idempotent but only creates)
- [ ] Open question: pad hidden in C++ the instant it closes would cut the close animation; if the animation needs the mesh until it ends, hide on an animation-finished call instead

## Verify
- [ ] Build, `Tools\Run_Tests.bat GeoTrinity.Encounter`
- [ ] `Runtime/pie_tutorial_flow.py` adapted (pad opens instead of door), headless
