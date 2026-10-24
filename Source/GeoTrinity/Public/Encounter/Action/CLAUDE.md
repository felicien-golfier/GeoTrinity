# Encounter/Action

Stock actions (base: `../GeoEncounterAction.h`). Each is a C++ class and a base for Blueprint children. They run on the authority (`../CLAUDE.md`, "Invariants").

| File | Does | Latent |
|---|---|---|
| `GeoDelayAction.h` | Waits `Duration` s (world time: holds during the solo pause) | yes |
| `GeoAddToCounterAction.h` | `AddToCounter(Name, Delta)` on its encounter | no |
| `GeoStartEncounterAction.h` | Starts another `AGeoEncounter` through its arrival reset (`StartAfterSkippingChain`: aborts the other running encounters, replays the skipped chain, starts at step 0; `../CLAUDE.md`, "Arrival reset"). One already running starts over. Last in its list: it aborts the encounter it sits in, so nothing after it runs (`IsDataValid` error) | no |
| `GeoSetTeleporterOpenAction.h` | `SetOpen(bool)` on the referenced `AGeoTeleporter` (a room's exit / entry pad) | no |
| `GeoActivateAbilityAction.h` | Activates `AbilityTag` on the boss of the referenced `AGeoArena` (a room's hidden caster, or a real boss), with optional `PatternData` for a pattern ability | yes |
| `GeoCatLineAction.h` | `ShowCatLine(Line, OnEnded)` on the first local player's HUD; `bWaitForRespawn`: while that player is dead, shown on its `OnRevived`; finishes when the line ends (typed + read, or F twice, `HUD/Tutorial/CLAUDE.md`) | yes |
| `GeoUnlockAbilityAction.h` | Grants the local player's class ability `AbilityTag` (`GiveClassAbility`); nothing the second time. Never opens the page. `OnSkipped` = `Execute` | no |
| `GeoShowAbilityPageAction.h` | `ShowAbilityPage(AbilityTag)` on the local HUD; placed after a Cat line, so the page never pops before the Cat speaks (`Tutorial/Steps.md`, "Unlock rule"). Finishes when the player closes the pause menu (`AGeoPlayerController::OnPauseMenuClosed`), at once with no HUD or no page; what follows never plays behind the page. `OnCancel` stops waiting only (the page stays open; its close no longer finishes the action). No-op when skipped | yes |
| `GeoSpawnEnemiesAction.h` | Keeps `Count` of `EnemyClass` + `Setup` alive on the points tagged `PurposeTag` + `ArenaTag`: spawns them, replaces each dead one `RespawnDelay` s after its own death, where it died (0: never); `bKillOnStepExit` kills the living when the step is left | no (held until the step is left) |
| `GeoSpawnZoneAction.h` | Spawns one effect zone (inline effects, size, colours, attitude, team) on the point tagged `PurposeTag` + `ArenaTag`; expires it when the step is left | no (held until the step is left) |
| `GeoQuestCardAction.h` | Quest card on the local HUD: `Show` (title, max, `TimeLimitSeconds`: decorative countdown), `Count` (payload `Count`, or `Max` with `bShowMax`), `Hide` | no |
| `GeoOpenGemsPageAction.h` | Opens the Gems menu on the local HUD, on its Forge page (`bForge`) or its loadout | no |
| `GeoSetMovementLockedAction.h` | `bLocked`: locks / unlocks the first local player's movement (walk input; the dash is not granted yet in that room, `Tutorial/Steps.md`); same solo-host rule as `GetLocalHUD()`. Whoever locks unlocks: leaving the step does not. One lock, not a count (`UGeoEncounterSubsystem::bMovementLocked`): a lock while locked or an unlock while free does nothing, so a restarted step never stacks a lock. `OnSkipped` = `Execute` (a skipped lock then unlock leaves movement free) | no |
| `Cat/` | Orders to the placed `AGeoCat`: walk, roam, play a montage -> `Cat/CLAUDE.md` | yes (`bWaitForEnd`) |
| `GeoSetActorHiddenAction.h` | `SetActorHiddenInGame(bHidden)` on the referenced level actor (the Cat before its first montage, after its last) | no |
| `GeoLeaveTutorialAction.h` | `UGeoGameInstance::LeaveTutorial()`: the tutorial's exit to the Training Room, the pause menu's path | no |

## Local HUD
- `GetLocalHUD()` (base): the first local player's `IGeoHUDInterface`. The tutorial is a solo listen host, so authority and UI share a process; on a remote client nothing is shown (encounters are authority-only).

## GeoCatLineAction
- Latent: lines in a list play one after the other, each ends before the next starts; a placed encounter needs no `GeoDelayAction` after a line. The HUD's end callback calls `Finish()`; no local HUD = `Finish()` at once.
- A rule's actions run to the end before it fires again (`../CLAUDE.md`): a quip rule ignores firings while its line is still on screen.
- `OnCancel`: unbinds the callback; step left while the line shows = the line stays on screen until it ends. On abort (`IsAborting()`) it also ends the line it shows (`HideCatLine`, only when that line is still the shown one).
- A death line waiting for the respawn (`bWaitForRespawn`) finishes after the respawn and the line.

- Showing a line broadcasts `UGeoEncounterSubsystem::OnCatLine` with the shown text, also with no local HUD: what a test reads, never the widget.
- `{Counter:<Name>}` in `Line` is replaced by that step counter of the action's encounter when the line shows (`GeoLib::ReplaceCounterTokens`; tested in `GeoTrinity.Encounter.CatLine`); the markup of `HUD/Tutorial` never sees it.

## GeoSpawnEnemiesAction
- Finishes at once; `AGeoEncounter::HoldUntilStepLeft` runs its `OnCancel` when the step is left (any way: completion, restart, abort, end of play), so the list goes on and respawns last until then. Run again while held (a repeating rule): its previous run ends first, as a cancel does (respawns stopped, the living killed with `bKillOnStepExit`; those deaths reach the step's armed rules).
- Spawned at `ArbitraryCharacterZ`, owned by the encounter, no arena (a dummy is no boss); first spawns take the points round robin (`Count` = number of points puts one on each).
- Respawn is per dummy, never a shared tick: its death (the character's death delegate, as `GeoActorDiedTrigger`) records its location and starts its own `RespawnDelay` timer; at expiry a new one is spawned there. The others are untouched, so nothing refills in a wave. Timers on world time (they hold during the solo pause); all dropped in `OnCancel`. Destroyed without dying (cancel, wipe): no respawn.
- Every spawn goes through `AEnemyCharacter::SpawnEnemy` (`Characters/CLAUDE.md`, "Enemy setup"). The variant (chaser, shooter, hurt ally, ally team) lives in `Setup` on the action, `EnemyClass` stays `BP_Dummy`.
- A one-off character of a room (the hurt ally, the rude dummy) is spawned too, `Count` 1, never placed: its `Setup.Tag` is what its triggers watch (`FGeoActorFilter`, `Trigger/CLAUDE.md`).

## GeoSpawnZoneAction
- A zone that only teaches its colour or sits in a room: no ability, no pattern, no caster. A telegraphed or patterned hazard stays an ability fired by `GeoActivateAbilityAction`.
- `DefaultZoneClass` (`UGameDataSettings`) through `GeoASLib::FullySpawnDeployable` with an `FDeployableData` filled from the action: `EffectDataArray`, `Params` (`Size`, `Color`, `SecondaryColors`, `Attitude`; `LifeDrainMaxDuration` 0 = lasts until cancelled), `TeamID` = `Team`, no `Owner`: self-owned, its own ASC is the effect source, like a hand-placed zone (`AbilitySystem/Lib/CLAUDE.md`).
- A growing zone is `Params.GrowToSize` + `GrowSeconds` (`GeoEffectZone` grows from `Size`; replicated start time); the action adds nothing. The zone stops at `GrowToSize` and stays until cancelled.
- Several zones in one step: one action per zone in the step's `OnEnter`, each held until the step is left, so they all end together (the Zones room's colour step, `Tutorial/Steps.md` step 7).
- Ground Z (0), like every zone. One zone per action: several zones on one step = a step per zone, or a Blueprint child spawning several.
- `OnCancel`: `Recall()` on its zone (blink, then gone). Held like `GeoSpawnEnemiesAction`; run again while held, it recalls its previous zone first.

## GeoActivateAbilityAction
- Caster = `Arena->GetBoss()`: a level reference to the arena, since the caster is spawned at runtime.
- `PatternData` (optional, `TInstancedStruct<FPatternData>`): handed to a `UPatternAbility` as its pattern data (the action is the activation's `IPatternDataSource`, `AbilitySystem/Abilities/Base/CLAUDE.md`). A wave's safe spots live here (`FLateralWavePatternData`), so one wave ability serves every firing.
- `bWaitForEnd` true: `Finish()` when the activated ability ends (ASC ability-ended delegate, matched by spec handle); false: `Finish()` right after activation (overlapping waves).
- Refused activation (no boss, no ability with the tag, blocked): log to `LogGeoEncounter`, `Finish()`; the encounter never stalls on it.
- `OnCancel` (step left while waiting) cancels the ability and unbinds. One fired without waiting runs on past a step exit (overlapping waves); an abort (`IsAborting()`) cancels it too.

- `OnSkipped` (jump start, `../CLAUDE.md`): only `GeoUnlockAbilityAction` and `GeoSetMovementLockedAction` (table above) override it; everything else (lines, pads, spawns, cards, menus) is a no-op when skipped.
- An action does one thing and reads what it needs from the payload or its own properties; no action knows the step it sits in.
- Moving players, opening a way between rooms, raising a tag event: the room's `AGeoTeleporter` pads (`GeoSetTeleporterOpenAction`), and a StateTree task or Blueprint for an event (`../CLAUDE.md`, event bus). No action for them until a step needs one.
- A new gameplay verb (lock an ability, grant loot, open a menu page, say a Cat line) is an action in this folder, written with the feature that needs it, with its own doc row here.

## Cancel
What `OnCancel` leaves behind on abort (`../CLAUDE.md`, "Arrival reset"); an action not listed has nothing to undo.
| Action | `OnCancel` |
|---|---|
| Delay, Activate ability, Show ability page, Spawn enemies / zone | as written above |
| Cat line | on abort, ends its shown line while that line is still the shown one (also a line left standing by a normal step exit) |
| Quest card | `Show` hides the card it showed (once); `Count` / `Hide` nothing |
| Set actor hidden | a show hides the actor again (actors are placed hidden); a hide has nothing to undo |
| `Cat/` orders | on abort, drops the cat's order (`Cat/CLAUDE.md`) |
| Movement lock | nothing: the skip replay of the next start nets the lock (`../CLAUDE.md`) |
| Unlock ability, pads, counters, Gems page, start, leave | nothing: lasting by design, or owned by the next room's `OnEnter` |
