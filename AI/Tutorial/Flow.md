# Flow

How the tutorial is reached, played and left. Steps themselves: `Steps.md`.

## Maps
- **Tutorial map**: its own level, one player, opened as a solo listen server (`UGeoSessionSubsystem::HostListen(TutorialMap)`): every authority gate (`GeoLib::IsServer`) is false in Standalone, so enemies would never die nor players respawn. No session is advertised. Map refs: `UGeoGameInstance::TutorialMap` and `TrainingRoomMap` (= `DraftMap`), soft refs next to `MainMenuMap`. Class forced by the level's world settings (`StartingClassOverride` = Circle, `World/CLAUDE.md`). Built on `AGeoEncounter` (`Source/GeoTrinity/Public/Encounter/CLAUDE.md`); the one player is the authority, so authority-only encounters and local-player things (gem profile, dialogue widget) live in the same process.
- Every tutorial room has an `AGeoArenaVolume` and `TargetPoint.Entrance` points for its own `Arena.*` tag: a death respawns at the room's entrance after `DeathTime` (GameState policy, `GameClasses/Match/CLAUDE.md`).
- **Training Room**: stays in the shared map (`DraftMap`), room tag `Arena.TrainingRoom`. The tutorial hands off to it. Its kit (zone abilities, their State Tree, the dummies' trees) is named `Training*` and the tutorial map reuses it; caster and dummies are native / `BP_Dummy` + setups on the room's arena, no Blueprint of their own. The room's top-left corner holds the Training Room Cat (`AI/Tutorial/Cat.md`), another add of that arena (`BP_TrainingCat` + setup).
- One player only: the map's game mode is `BP_TutorialGameMode` (child of `BP_GeoGameMode`, `AGeoGameMode::bSoloOnly` = true); `InitNewPlayer` then refuses a player when another player controller is already in the world (the new controller is in the list by then; `GetNumPlayers` counts only players past `PostLogin`, so it is not used). One rule for both a remote joiner (the listen port is open) and a second local player (gamepad press, `GeoGameViewportClient`): both pass through `Login`.

## Rooms
- Rooms are sealed by fixed walls and separated by empty space (`ROOM_GAP`, wide enough that the next room is never on screen); the only way on is the teleporter. No barrier, no door.
- The players learn the lobby's teleporter (`AGeoTeleporter`, `Actor/Buttons/CLAUDE.md`) here, so every pad is that one actor, same look and same open / close animation.
- Per room: an **exit** pad at its fixed spot, off the path of the room's task (a pad opening under the player would teleport them unasked) and, from room 2 on, an **entry** pad where the previous exit lands the player. All share one `TeleportTag`; names sort `Tut_Pad_<NN>_Entry` < `Tut_Pad_<NN>_Exit`, so exit N → entry N+1 → exit N+1 (the entry's own next is its room's exit: it must be closed before the player can walk back onto it).
- The first room (Landing, steps 0-1) has no entry pad; its exit pad is the step 1 target, opened by the last Move line, not by a finished task.
- Pads start closed. The entry pad never opens: it only marks where the previous exit lands the player, so no pad is on screen on arrival. The previous room's exit closes at once, when the next encounter starts (`GeoVolumeEnteredTrigger` on the arena volume). Exit opens with the room's last Cat line: its `GeoSetTeleporterOpenAction` sits just before that line, so the pad appears as the line shows (Shoot: with "Next toy") and the player may take it before the line ends; it closes on the next room's arrival. The **last built room** (the last of the script's `ROOMS`; rooms not built yet do not count) has a **final exit pad** instead: it opens like any exit pad but carries its own `TeleportTag` (`Teleport.TutorialExit`, shared with no pad, so it moves nobody), and a volume over it (`GeoVolumeEnteredTrigger`) runs `GeoLeaveTutorialAction` (Exit below). Building the next room hands the role on: the old pad returns to the shared tag, its Leave step comes back.
- **Arrival watched from every step**: every step of a room carries the arrival rule (next room's arena volume entered → close this exit, start the next encounter), not only its last `Leave` step. A pad taken while the room still runs (Landing: the Cat still walking into the portal) starts the next room at once.
- **Arrival reset**: taking a pad starts the next room's encounter, which resets the world to what that room expects: the room left is aborted (no `OnExit`; every action of its current step is cancelled and undoes what it left: enemies and zones gone, Cat line and quest card cleared, Cat order dropped), then abilities and locks the player should hold are replayed as in a jump. The Cat never finishes a sentence from the room behind. Mechanism: `Encounter/CLAUDE.md`, "Arrival reset".

## Menus pause
- While the pause menu is open (an ability page popped by an unlock, the Gems menu) the tutorial world is paused: encounter timers, delays, idle tiers, spawner refills and enemies all hold, and resume when it closes. Cat lines run on UI time, so a line said with the page shows above it and fades on its own. Rule and mechanism: `GameClasses/Player/CLAUDE.md` ("Solo pause").

## Entry
- Main menu button **Tutorial**, always there: opens the tutorial map.
- **First-launch prompt**: the first time the main menu opens (`GeoGameUserSettings` flag not set), a popup in the Cat's voice says the player SHOULD do the tutorial, and lets them refuse as "incompetent and ignorant". Two choices: do it (opens the tutorial map) or skip (closes the popup). Either answer sets the flag; the popup never shows again. Text: Tutorial Board, page "Prompt" (the artifact holds on-screen text until validated, `CLAUDE.md`).
- Skip is also available from inside the tutorial: the pause menu offers **Leave tutorial**, which goes straight to the exit below.

## Jump to a room (dev)
- Console `Geo.Tutorial.Jump <Room>`: a name (`Dash`, `Shoot`, `Deploy`, `Special`, `Dummies`, `Zones`, `Miniboss`) or the room's number, as in the outliner folders (`0` Landing, `1` Dash, `2` Shoot…) = `UGeoGameInstance::OpenTutorial(Room)` = `HostListen(TutorialMap, Room)`: reloads the tutorial map with the room as portal. Works from the menu, in the tutorial and in PIE.
- Per room from Dash on: a `PlayerStart` on its entry pad, `PlayerStartTag` = room name, referenced by its encounter's `JumpStart`. The player spawns there; that room's encounter starts, Landing's does not.
- Earlier rooms are skipped, not played: their `OnEnter` / `OnExit` replayed in order without effects but the lasting ones (abilities unlocked silently, movement lock / unlock pairs), mechanism `Encounter/CLAUDE.md` ("Jump start"). The target room runs normally (its own unlock page pops); its entry pad stays closed, like every entry.
- Gems: nothing granted by the skip; the real profile already holds past runs' loot.

## Exit
- When no next step exists, the final exit pad (Rooms) is the way out: stepping on it leaves for the Draft map's Training Room, same as **Leave tutorial**.
- On the last step, or on **Leave tutorial**: `UGeoSessionSubsystem::HostListen(TrainingRoomMap, "TrainingRoom")`: the portal travels in the URL (`DraftMap?listen#TrainingRoom`) and the engine's `Login` → `FindPlayerStart(Portal)` picks the `PlayerStart` whose `PlayerStartTag` is `TrainingRoom` (non-seamless travel; no game-mode code). The player is then host of a normal listen server; others join through the usual create / join flow (`System/CLAUDE.md`).

## Replay and loot
- Replayable from the menu any number of times. The scripted loot (one chip of each kind), the Forge shard grant and the miniboss's normal class XP go to the real gem profile on every run; no anti-farm, no temporary profile. The amount is small by design.
- Nothing is persisted about progress inside the tutorial: a quit mid-way restarts it at step 0.
