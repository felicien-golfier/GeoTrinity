# Tests/Tutorial

Headless play-in-editor runs of the real `/Game/Maps/Tutorial`: a bot plays the tutorial and checks what each room must show. Editor module because play-in-editor needs `UnrealEd` / `LevelEditor`. How to run, the gate: `AI/Testing.md`, "Tutorial runs".

| File | Role |
|---|---|
| `GeoTutorialBot.h/.cpp` | `FGeoTutorialBot`: latent command running a list of stages, one per frame; world / pawn / pad / enemy / zone helpers, the Cat line and page logs, ability watches, per-frame observers, `JumpTo(Room)`, `QueueSession` (load map, play session, bot, end, restore profile). Also `FGeoStartListenPlayCommand`, `FGeoRestoreGemProfileCommand` |
| `GeoTutorialRoomStages.h/.cpp` | `GeoTutorialStages::Add<Room>Stages(Bot, branch)` per room and branch: its stages, mirroring the room's script (`AI/Python/Tutorial/Level/tutorial_encounters.py`, whose line fragments sit at the top of the `.cpp`); the caller adds the start stage and the teleports |
| `GeoTutorialRunTests.cpp` | `GeoTrinity.Tutorial.FullRun`: Landing to Miniboss in one session, real pads, the main branch of each room |
| `GeoTutorialPathTests.cpp` | Complex tests: `GeoTrinity.Tutorial.Path.<Case>` (every other branch, one session each, entered by `JumpTo`) and `GeoTrinity.Tutorial.Jump.<Room>` (the console jump to each room by name, real travel) |

## Bot
- Stage = name + lambda `bool(bFirstTick)`; true moves on. Each stage has a real-time timeout naming it; a dummy that never spawns stalls its spawn stage, an extra one fails at once.
- Every frame: closes pause menus (after noting how many class abilities the player held and whether the world paused: one unlock, one page), ends Cat lines (`HideCatLine`; `KeepLineFragment` leaves one standing: `Path.EarlyPortal`), fails if an entry pad opens. 8x time dilation.
- `Geo.PlayerInvincible` on, except where the room needs the hit: falls, lethal and safe-spot waves, wipes. Allies still die.
- Line log: every line from `UGeoEncounterSubsystem::OnCatLine`, in order. A play world is subscribed the moment it initialises (`FWorldDelegates::OnPostWorldInitialization`), so a travel's first line, shown in `BeginPlay`, is not missed. A branch is proved by its line (stunned vs relief, sting vs surrender, quip at 3 / 6 / 9, tips in order), never by its text typed in the test: the test holds a short unique fragment of the script's line.
- Counters: `AGeoEncounter::GetCounter` reads the room's tally (`Falls`, `Progress`, `Dead`, `WavesDone`; step-scoped, so read while the step runs). Ability watches (`WatchActivations`) count what an actor's ASC activates and follow a respawned actor; observers run every frame (boss points visited). `bKeepPlayerHealthy` refills the pawn each frame in the Miniboss branches (a stray bullet kills nothing); `KillPlayer` drops the invincibility for the one `Death()` only.
- Kills through `Death()` (the Miniboss's last hit sets its health to 0: only that path reaches the arena's defeat and loot); real abilities where headless allows: Dash (placed at the void edge facing +X, then `TryActivateAbility`), Healing Zone (`GeoASLib::SpawnDeployableFromAbility` on the ally), Special (`TryActivateAbility`). Health set through the attribute to hurt (call-outs, the Heal zone check) and in the Special room, where the beam cannot be aimed headless.
- `JumpTo(Room)`: the world-URL jump without the travel: pawn onto the `PlayerStart` tagged with the room name, then `StartAfterSkippingChain()` on `Tut_<Room>_Encounter`. The real console travel is `Jump.<Room>`; it, `FinalExit`, `LeaveFromMenu`, `Replay` and the `Jump*` paths wait for a new play world (PIE server travel).
- The map's `StartingTutoRoomInEditor` (editor quick-start room, a jump that replays the unlocks before it) is cleared on the loaded editor map before play, in memory only: every session starts at Landing or at its own `JumpTo`.
- Expected, never failed on: `GC_Explosion`'s Blueprint error (headless: no Niagara component to set a variable on).
- A kept line (`KeepLineFragment`) is never ended by the bot: the arrival's abort ends it (`Path.EarlyPortal`).
- Waves after a death: the run before still fires its remaining waves while its Cat line waits for the respawn, so the safe-spot bot re-bases its wave count when the pawn is alive again. The Miniboss wipe stages walk the respawned pawn back into the arena before each death.
- The play-in-editor world begins play before its player joins: its start stage clears the line log, then a FullRun restarts `Tut_Landing_Encounter` once the pawn exists; a path test jumps instead.
- The Miniboss kill writes XP and loot to the user's gem profile: every session that can kill it loads the profile first and writes it back after (`FGeoRestoreGemProfileCommand`).
- Headless `-NullRHI` spawns no Niagara component; `UPattern::SpawnPersistentVfx` ensures only where the game can render.

## Coverage
Every branch of `tutorial_encounters.py` has one owner. A new room, step, rule or branch adds its stages and a row here in the same change.

| Room | Branch | Test |
|---|---|---|
| all | entry pads never open; exit closes on arrival; each unlock pops its page, world paused while it shows | FullRun |
| all | console `Geo.Tutorial.Jump <Room>` travels; pawn on `JumpStart`, earlier abilities granted (and Deploy's / Special's own, they unlock as their step starts), Landing never runs, room's step 0 starts | Jump.<Room> |
| all | jump by number (`Geo.Tutorial.Jump 6`): Zones | Path.JumpByNumber |
| all | jump by a number past the chain: fresh start at Landing (locked, no class ability) | Path.JumpPastEnd |
| Landing | locked until the WASD line, then free; Cat shown, then hidden after its portal jump | FullRun |
| Landing | idle on the open pad: the nag once, then `Pun` jokes | Path.LandingNag |
| Landing | pad taken during the last line, Cat still on its way to the portal: the Dash room starts at once (its first line while the Landing step still ran), the Landing line and the Cat stop, no Landing line after | Path.EarlyPortal |
| all | each room's exit pad opens with its last Cat line (Shoot: "Next toy"), from Dash on (`WatchExitPadsOpenWithLastLine`; Landing: Path.EarlyPortal) | FullRun |
| Dash | real dash over the void, 0 falls: stunned line | FullRun |
| Dash | falls 1-7: each respawns on the room's entrance; first, second, 3-5, 6+ lines; reach: relief line with the count | Path.DashFalls |
| Dash | idle tiers 1-4, each once, then `Pun` jokes from `Data/CatLines.txt` (two in a row) | Path.DashIdle |
| Shoot | first dummy, delay, nine; quips at 3 / 6 / 9; a dead one respawns where it died; 12 kills cut; dummies removed | FullRun |
| Shoot | 45 s cut before 12 kills | Path.ShootCut |
| Shoot | pause menu open: respawns and the cut hold | Path.PauseHolds |
| Deploy | ally starts hurt; real Healing Zone heals it full; pad | FullRun |
| Special | two stings, then Special: done | FullRun |
| Special | Special in round 1: done, round 2 never spawns | Path.SpecialFirstTry |
| Special | Special first, ally full, then kill; ally dips and is healed again: counted once | Path.SpecialOrder |
| Special | three rounds without: surrender line, pad opens | Path.SpecialSurrender |
| Dummies | chaser closes in, shooter fires; three kills | FullRun |
| Dummies | death mid-fight: respawn on the entrance, survivors stay, fight goes on | Path.DummiesDeath |
| Zones | Damage zone grows from the centre to cover the room and hurts; then Shield and Heal spawn, the pawn stands in each, 2 s later every zone is gone (a step into a spot before its zone is up counts nothing); lethal wave kills | FullRun |
| Zones | safe spots: series 1 dies on wave 2, series 2 on wave 3, series 3 clears; every lane of every wave used once; retry line, restart from wave 1 | Path.SafeSpotsRetry |
| Miniboss | salve, boss moves between its points, summon's zone grows, killed summon turns ally and fires its own salve (`GA_TutorialAllySalve`), wave, last hit (health to 0, as damage does) opens the final exit, loot and XP granted; the fountain throws one drop per granted gem and shard | FullRun |
| Miniboss | deaths 1-3: wipe (adds gone, boss full), tips in order; 4th death silent | Path.MinibossWipe |
| Miniboss | ally under 30 %: call-outs 1-3, then silent | Path.MinibossCallouts |
| Miniboss | wave: in a living dummy's lane survives, out of it dies; no dummy alive: dies anywhere | Path.MinibossWaveLanes |
| Miniboss | step on the final exit: Training Room, on its `TrainingRoom` start | Path.FinalExit |
| any | pause menu Leave tutorial: Training Room | Path.LeaveFromMenu |
| any | Leave tutorial, then the tutorial opened again: fresh start at Landing, nothing kept | Path.Replay |

Covered elsewhere: solo-only login (`GeoTrinity.Tutorial.SoloOnly`), locked / unlocked abilities, encounter mechanics (`GeoTrinity.Encounter.*`). Not played: the Heal and Shield zones' effect (the bot stands in them, the effect is not measured), the Dummies room's mover patrol (not built, `AI/Pending/Tutorial.md`), the Miniboss wave count after a wipe (tree state), quest card display and line typing (UI module, no probe), first-launch prompt and menu button (main menu, no tutorial world), steps 11-13 (not built).
