# AI/Python/Level

Level content and per-level settings. Doc: `AI/MCP/MCP_Settings.md`.

| Script | Does |
|---|---|
| `arena_walls.py` | Place arena wall and barrier states on a floor outline, each turned so local +X is its travel direction (`AI/ArenaWalls.md`) |
| `level_settings.py` | World settings, GameMode override, controller class |
| `tutorial_map.py` | `BP_TutorialGameMode`, the game instance map refs, the Training Room `TrainingRoom` PlayerStart, `/Game/Maps/Tutorial` (8 contiguous rooms along +X: floor, walls, arena + camera volume, `TargetPoint.Entrance`, fixed end walls, a `ROOM_GAP` of empty space between rooms, the Dash room's void: near floor, far floor, `Tut_Dash_PlatformVolume`, an entry and an exit `AGeoTeleporter` per room, from Dash on a `PlayerStart` on the entry pad tagged with the room name, `AI/Tutorial/Flow.md` "Rooms", "Jump to a room"); headless, delete `Tutorial.umap` to rebuild |
| `tutorial_assets.py` | `CT_Tutorial` (curve table of every tutorial number) and the shared dummy StateTrees `/Game/AI/Dummy/ST_Dummy_Shooter`, `ST_Dummy_Chaser` (no arena gate); enemy variants are setups in `tutorial_encounters.py`, no Blueprints |
| `tutorial_encounters.py` | Rooms 0-5 (Landing holds steps 0-1): spawn and zone points (never floors: the map script owns all geometry, the Dash void included) (allies and dummies are spawned with a setup, never placed), one chained `AGeoEncounter` per room with the Board's Cat lines (a line is latent: no wait after it), `JumpStart` = its room's `PlayerStart`; lasting actions (unlock, movement lock) in `OnEnter` / `OnExit`, never in a rule; pad open / close actions; Landing's `BP_Cat` (`Tut_Landing_Cat`, placed hidden) and its path points, staged by the Cat actions (walk, roam, montage) and `GeoSetActorHiddenAction` (`AI/Tutorial/Steps.md`, "Cat staging") |
| `encounters_diff.py` | Read-only: diff every tutorial room's steps and the spawn points, scene vs what `tutorial_encounters.py` would write → `AI/Output/encounters_diff.txt` (`-` scene, `+` script); run through `Runtime/run_via_bridge.py` |
| `training_room.py` | Build the Training Room (`Arena.TrainingRoom`) and its `Training*` kit in the draft map |

## Read the scene first (rule)
Before editing or re-running `tutorial_encounters.py`, run `encounters_diff.py`: the scene is the truth, the script follows it. Every `-` line is a hand edit to carry into the script (then re-run until the diff holds only intended changes); only then re-run the script.

## Hand edits survive a re-run (rule, `tutorial_map.py` and `tutorial_encounters.py`)
Same rule as `AI/UI.md` ("A script never overwrites a hand edit"), same ledger (`UI/asset_guard.py`, `asset_values.json`), keyed by actor path + property.
- `tutorial_map.py` too: room floors, walls, camera / arena volumes, pads, arrival zones, `PlayerStart`s, entrance points, world settings, game mode defaults; a prune of the DraftMap copy (`create_map`) and the `Removed_` clean-up in `build_rooms` only touch actors the script placed and nobody moved.
- Every value the script sets on a level actor (location, rotation, scale, tags, folder, `Steps`, `bStartOnBeginPlay`) goes through `asset_guard.write`; a value changed by hand is kept and listed in the report (`kept by hand`).
- `Steps` is guarded as one property: hand-edit any rule of a room and that whole room's `Steps` is kept; the script's version is then not applied until the user carries the change into the script or says to overwrite (delete the ledger entry).
- An actor found by label is never moved or rescaled unless its transform still equals what the script last wrote; an actor missing is spawned (the script's to place).
- Never destroyed by hand-edit check: `remove_actor` only removes actors an earlier script placed and the user has not touched.
- The folder rule below already skips an actor with a folder.

## Camera volumes (rule)
Any room a script builds sets a `GeoCameraVolume`'s `TriggerBox` AND `BoundsBox` extents to the room's half size, actor scale 1 (`size_camera_volume` in `tutorial_map.py`): never a scaled actor, never a margin, so the camera frames exactly the room. Re-running `build_rooms` resizes the existing volumes.

## Map folders (rule)
Every actor a script places in a map goes into an outliner folder in the same script, never left at the root. Tutorial map: `Tutorial/Rooms/<NN_Room>/<Category>` (`Encounter`, `Doors`, `Arena`, `Points`, `Gameplay`, `Geometry`), camera volumes under `Tutorial/Camera/<NN_Room>`, then global `Tutorial/Camera`, `Lighting`, `Layout`. A new kind of actor adds its rule to `CATEGORIES` in `tutorial_encounters.py` (`organize_folders` reports any actor it cannot place). Same for any other map.
