# AI/Python/Tutorial

Everything that builds or checks the Tutorial: its map, data, encounters, HUD / menu widgets, headless flow checks. Game side: `AI/Tutorial/` (`Flow.md`, `Steps.md`), `Source/GeoTrinity/Public/Encounter/CLAUDE.md`. Its Cat: `AI/Python/Cat/`.

## Level/
Level content. Doc: `AI/MCP/MCP_Settings.md`.

| Script | Does |
|---|---|
| `tutorial_map.py` | `BP_TutorialGameMode`, the game instance map refs, the Training Room `TrainingRoom` PlayerStart, `/Game/Maps/Tutorial` (8 contiguous rooms along +X: floor, walls, arena + camera volume, `TargetPoint.Entrance`, fixed end walls, a `ROOM_GAP` of empty space between rooms, the Dash room's void: near floor, far floor, `Tut_Dash_PlatformVolume`, an entry and an exit `AGeoTeleporter` per room, from Dash on a `PlayerStart` on the entry pad tagged with the room name, `AI/Tutorial/Flow.md` "Rooms", "Jump to a room"); headless, delete `Tutorial.umap` to rebuild |
| `tutorial_assets.py` | `CT_Tutorial` (curve table of every tutorial number) and the shared dummy StateTrees `/Game/AI/Dummy/ST_Dummy_Shooter`, `ST_Dummy_Chaser` (no arena gate); enemy variants are setups in `tutorial_encounters.py`, no Blueprints |
| `tutorial_encounters.py` | Rooms 0-7 (Landing holds steps 0-1, Zones steps 7-9, Miniboss step 10 with its points, wave start and `AGeoArena` carrying the scripted loot, `BossClass` `BP_Dummy` + `BossSetup`): spawn and zone points (never floors: the map script owns all geometry, the Dash void included) (allies and dummies are spawned with a setup, never placed), the Zones room's zone boxes and its `AGeoDummyArena` (`BossClass` `AHiddenCasterCharacter`, `BossSetup.AbilityTags` = the wave), one chained `AGeoEncounter` per room with the Board's Cat lines (a line is latent: no wait after it), `JumpStart` = its room's `PlayerStart`; lasting actions (unlock, movement lock) in `OnEnter` / `OnExit`, never in a rule; pad open / close actions (the entry pad never opens, no arrival volume: the next room's `Leave` rule closes the exit, then starts the next encounter: the start goes last, it aborts the room); unlock = the Cat's page line, `GeoUnlockAbilityAction`, `GeoShowAbilityPageAction` (`Builder.unlock`); Shoot's lone first dummy on `Tut_Shoot_FirstDummyPoint` (room centre), a `GeoDelayAction` (`FIRST_DUMMY_DEATH_SECONDS`) before the quest, its nine respawn points the hand-placed `SHOOT_SPAWNS` table; a room's Cat lines, spawns and unlock sit in `OnEnter` when the scene has them there (Deploy is one step; only the Dash room says the page line); what happens as a step starts (lines, spawns, a wait) is `OnEnter` with `delay()`, never a trigger; Special: `Round1`-`Round3`, each respawning both dummies and tallying `Progress` (`GeoAbilityActivatedTrigger`; `AI/Tutorial/Steps.md`, step 5); the last room of `ROOMS` ends on `exit_steps` (final exit pad: own tag, `Tut_<Room>_PadExitVolume` → `GeoLeaveTutorialAction`) instead of `leave_steps`; Landing's `BP_Cat` (`Tut_Landing_Cat`, placed hidden) and its path points, staged by the Cat actions (walk, roam, montage) and `GeoSetActorHiddenAction` (`AI/Tutorial/Steps.md`, "Cat staging") |
| `tutorial_miniboss.py` | The miniboss's assets, only what holds a look or behaviour (`AI/CodingStyle.md`, "Variants are data"): its behaviour tree `ST_TutorialMiniboss` (copy of the star boss's, emptied and rebuilt), `BP_TutorialSummonedDummy` (zone and ally params; zone growth from `CT_Tutorial` rows), `GA_TutorialSalve` + `BP_TutorialPattern_Salve` (one salve for boss, summons, allies), `GA_TutorialMinibossSummon` (pattern = the Training Room's `BP_ZonePattern_BurstDamage`; `AddSetup` = the summon's health row, `ST_TutorialSalveShooter` (`ST_Dummy_Shooter` firing the salve), salve), catalog entries. The boss itself is `BP_Dummy` + the Miniboss arena's `BossSetup` (`tutorial_encounters.py`); no health effect copies, health is a setup's `MaxHealth` row. Idempotent, headless, after `tutorial_assets.py` and `tutorial_waves.py`; deletes the assets of the old layout |
| `tutorial_waves.py` | The one lateral-wave ability (`BP_LateralWave` pattern + `GA_LateralWave`, tag `Ability.Spell.LateralWave`, registered in `DA_AbilityInfo`); each firing's safe spots are the encounter action's `PatternData` (`tutorial_encounters.py`, `Builder.wave`), the miniboss's come from its adds; its caster is the Zones arena's native `AHiddenCasterCharacter` + `BossSetup` (`tutorial_encounters.py`); idempotent, headless |
| `encounters_diff.py` | Read-only: diff every tutorial room's steps and the spawn points, scene vs what `tutorial_encounters.py` would write → `AI/Output/encounters_diff.txt` (`-` scene, `+` script), and every room's whole scene steps → `AI/Output/encounters_scene.txt`; run through `AI/Python/Runtime/run_via_bridge.py` |
| `scene_dump.py` | Read-only: every actor of the Tutorial level (class, label, folder, parent, tags, transforms, components, properties off the class default) → `AI/Output/tutorial_scene.txt`; with `encounters_diff.py` it is the scene side of the `sync-tutorial-scripts` skill; run through `AI/Python/Runtime/run_via_bridge.py` |
| `training_room.py` | Build the Training Room (`Arena.TrainingRoom`) and its `Training*` kit in the draft map: StateTrees, the room, one `AGeoDummyArena` (`BossClass` `AHiddenCasterCharacter` + `BossSetup` = zones tree and the six zone abilities; `StartAdds` = `BP_Dummy` + a setup per station); zones tinted by their pattern's `ZoneParams.Color`, no zone Blueprints or material copies |

## UI/
Widgets. Docs: `AI/UI.md`, `AI/MCP/MCP_UI.md`; every write goes through `AI/Python/UI/asset_guard.py`.

| Script | Does |
|---|---|
| `tutorial_menu.py` | `WBP_TutorialPrompt`, `TutorialButton` + prompt page in the main menu, `LeaveTutorialButton` in the pause menu; headless, after `AI/Python/UI/menu_pages.py` |
| `tutorial_hud.py` | `WBP_CatLine`, `WBP_QuestCard`, `DT_CatLine`, `CatLineWidgetClass` / `QuestCardWidgetClass` on `BP_GeoHudMain`; headless, after `AI/Python/UI/ui_theme.py` |

## Runtime/
Headless checks, `-game -RenderOffScreen` (no window, no focus). Not `-nullrhi`: it spawns no Niagara component, and the zone room's wave patterns (and the map's hidden caster, from level start) need theirs. Docs: `AI/MCP/MCP_PIE.md`, `AI/Testing.md`.

| Script | Does |
|---|---|
| `pie_leave_tutorial.py` | Standalone-style game (`py` through `-ExecCmds`): travel Tutorial → Training Room and report where the player spawns |
| `pie_tutorial_jump.py` | Jump start + room teleport: game started at `Tutorial?listen#Zones`, steps into each zone box, dies to the lethal wave, waits past the safe-spot waves' reach (they hit by centre only), steps on the Zones exit pad, reports it landed in the Miniboss room; Deploy / Special / Dummies are not played (they need a heal or a fight) |
| `pie_tutorial_miniboss.py` | Miniboss room (`#Miniboss`, `Geo.PlayerInvincible`): logs the boss against its 3 points, summoned dummies, growing zone, ally, wave strips; kills a summon then the boss by KillZ (never by damage: a real kill writes the user's gem profile), steps on the final exit pad, reports the Training Room landing |
| `pie_tutorial_flow.py` | Check of the tutorial chain: moves the pawn, kills dummies under KillZ, steps on the exit pads (real teleports), reports pad openings (an entry never opens), the exit closing on arrival and the 3D Cat's visibility Landing → Shoot; cuts each Cat line's reading time (`LineSeconds`), since lines run on Slate time, not slomo |

## Read the scene first (rule)
Before editing or re-running `Level/tutorial_encounters.py`, run `Level/encounters_diff.py`: the scene is the truth, the script follows it. Every `-` line is a hand edit to carry into the script (then re-run until the diff holds only intended changes); only then re-run the script.

## Hand edits survive a re-run (rule, `tutorial_map.py` and `tutorial_encounters.py`)
Same rule as `AI/UI.md` ("A script never overwrites a hand edit"), same ledger (`AI/Python/UI/asset_guard.py`, `AI/Python/UI/asset_values.json`), keyed by actor path + property.
- `tutorial_map.py` too: room floors, walls, camera / arena volumes, pads, `PlayerStart`s, entrance points, world settings, game mode defaults; a prune of the DraftMap copy (`create_map`) and the `Removed_` clean-up in `build_rooms` only touch actors the script placed and nobody moved.
- Every value the script sets on a level actor (location, rotation, scale, tags, folder, `Steps`, `bStartOnBeginPlay`) goes through `asset_guard.write`; a value changed by hand is kept and listed in the report (`kept by hand`).
- `Steps` is guarded as one property: hand-edit any rule of a room and that whole room's `Steps` is kept; the script's version is then not applied until the user carries the change into the script or says to overwrite (delete the ledger entry).
- An actor found by label is never moved or rescaled unless its transform still equals what the script last wrote; an actor missing is spawned (the script's to place).
- Never destroyed by hand-edit check: `remove_actor` only removes actors an earlier script placed and the user has not touched.
- The folder rule below already skips an actor with a folder.

## Trigger boxes (rule)
A `TriggerBox` is the size of the element it watches, nothing smaller (a gap to dash past or stand beside), nothing larger (it must not reach into the next room): the dash landing = the whole far floor (`size_box` in `tutorial_map.py`), a colour zone = its bounding square (`ZONE_RADIUS`), an exit pad volume = the pad's footprint. Set the extent on the box's `BoxComponent` with actor scale 1 (`size_box`, one helper in `tutorial_map.py`, imported by `tutorial_encounters.py`): never a scaled actor, since a scale multiplies the default extent (40, not 100). Height: a standing capsule is inside (`ZONE_BOX_HALF_Z`, `PLATFORM_VOLUME_*`).

## Camera volumes (rule)
Any room a script builds sets a `GeoCameraVolume`'s `TriggerBox` AND `BoundsBox` extents to the room's half size, actor scale 1 (`size_camera_volume` in `tutorial_map.py`): never a scaled actor, never a margin, so the camera frames exactly the room. Re-running `build_rooms` resizes the existing volumes.

## Map folders (rule)
Every actor a script places in a map goes into an outliner folder in the same script, never left at the root. Tutorial map: `Tutorial/Rooms/<NN_Room>/<Category>` (`Encounter`, `Doors`, `Arena`, `Points`, `Gameplay`, `Geometry`), camera volumes under `Tutorial/Camera/<NN_Room>`, then global `Tutorial/Camera`, `Lighting`, `Layout`. A new kind of actor adds its rule to `CATEGORIES` in `tutorial_encounters.py` (`organize_folders` reports any actor it cannot place). Same for any other map.
- `<NN>` = the room's index in `ROOMS`: contiguous, unique, in room order (outliner sorts by name). Adding, removing or reordering a room renumbers every later room, so its actors' `NN_Room` folders (`Rooms/` and `Camera/`) are renamed in the same change, never left beside the new ones.
- Every folder pass ends with a check: each room has exactly one `NN_<Room>` folder per parent (`Rooms/`, `Camera/`), no two folders share an `NN` or a room name, `NN` matches `ROOMS.index`, no empty `NN_` folder remains; any violation is reported, never silently kept. A folder an actor already has is skipped by `organize_folders`, so this check is what catches a stale one.
