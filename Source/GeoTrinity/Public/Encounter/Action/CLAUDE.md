# Encounter/Action

Stock actions (base: `../GeoEncounterAction.h`). Each is a C++ class and a base for Blueprint children. They run on the authority (`../CLAUDE.md`, "Invariants").

| File | Does | Latent |
|---|---|---|
| `GeoDelayAction.h` | Waits `Duration` s | yes |
| `GeoAddToCounterAction.h` | `AddToCounter(Name, Delta)` on its encounter | no |
| `GeoStartEncounterAction.h` | Starts (or restarts) another `AGeoEncounter` | no |
| `GeoSetTeleporterOpenAction.h` | `SetOpen(bool)` on the referenced `AGeoTeleporter` (a room's exit / entry pad) | no |
| `GeoActivateAbilityAction.h` | Activates `AbilityTag` on the boss of the referenced `AGeoArena` (a room's hidden caster, or a real boss) | yes |
| `GeoCatLineAction.h` | `ShowCatLine(Line, OnEnded)` on the first local player's HUD; `bWaitForRespawn`: while that player is dead, shown on its `OnRevived`; finishes when the line ends (typed + read, or F twice, `HUD/Tutorial/CLAUDE.md`) | yes |
| `GeoUnlockAbilityAction.h` | Grants the local player's class ability `AbilityTag` (`GiveClassAbility`), then `ShowAbilityPage`; nothing the second time. `OnSkipped`: grants, no page | no |
| `GeoSpawnEnemiesAction.h` | Keeps `Count` of `EnemyClass` + `Setup` alive on the points tagged `PurposeTag` + `ArenaTag`: spawns them, replaces each dead one `RespawnDelay` s after its own death, where it died (0: never); `bKillOnStepExit` kills the living when the step is left | yes, until cancelled |
| `GeoSpawnZoneAction.h` | Spawns one effect zone (inline effects, size, colours, attitude, team) on the point tagged `PurposeTag` + `ArenaTag`; expires it when the step is left | yes, until cancelled |
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
- `OnCancel` (step left while the line shows) unbinds the callback only; the line stays on screen until it ends.
- A death line waiting for the respawn (`bWaitForRespawn`) finishes after the respawn and the line.

## GeoSpawnEnemiesAction
- Never finishes on its own: last in its action list (later ones would wait forever); the step ends through another rule, which cancels it.
- Spawned at `ArbitraryCharacterZ`, owned by the encounter, no arena (a dummy is no boss); first spawns take the points round robin (`Count` = number of points puts one on each).
- Respawn is per dummy, never a shared tick: its death (the character's death delegate, as `GeoActorDiedTrigger`) records its location and starts its own `RespawnDelay` timer; at expiry a new one is spawned there. The others are untouched, so nothing refills in a wave. Timers on world time (they hold during the solo pause); all dropped in `OnCancel`. Destroyed without dying (cancel, wipe): no respawn.
- Every spawn goes through `AEnemyCharacter::SpawnEnemy` (`Characters/CLAUDE.md`, "Enemy setup"). The variant (chaser, shooter, hurt ally, ally team) lives in `Setup` on the action, `EnemyClass` stays `BP_Dummy`.
- A one-off character of a room (the hurt ally, the rude dummy) is spawned too, `Count` 1, never placed: its `Setup.Tag` is what its triggers watch (`FGeoActorFilter`, `Trigger/CLAUDE.md`).

## GeoSpawnZoneAction
- A zone that only teaches its colour or sits in a room: no ability, no pattern, no caster. A telegraphed or patterned hazard stays an ability fired by `GeoActivateAbilityAction`.
- `DefaultZoneClass` (`UGameDataSettings`) through `GeoASLib::FullySpawnDeployable` with an `FDeployableData` filled from the action: `EffectDataArray`, `Params` (`Size`, `Color`, `SecondaryColors`, `Attitude`; `LifeDrainMaxDuration` 0 = lasts until cancelled), `TeamID` = `Team`, no `Owner`: self-owned, its own ASC is the effect source, like a hand-placed zone (`AbilitySystem/Lib/CLAUDE.md`).
- Ground Z (0), like every zone. One zone per action: several zones on one step = a step per zone, or a Blueprint child spawning several.
- `OnCancel`: `Recall()` on its zone (blink, then gone). Same "last in its list" rule as `GeoSpawnEnemiesAction`.

## GeoActivateAbilityAction
- Caster = `Arena->GetBoss()`: a level reference to the arena, since the caster is spawned at runtime.
- `bWaitForEnd` true: `Finish()` when the activated ability ends (ASC ability-ended delegate, matched by spec handle); false: `Finish()` right after activation (overlapping waves).
- Refused activation (no boss, no ability with the tag, blocked): log to `LogGeoEncounter`, `Finish()`; the encounter never stalls on it.
- `OnCancel` (step left while waiting) cancels the ability and unbinds.

- `OnSkipped` (jump start, `../CLAUDE.md`): only the two rows above override it; everything else (lines, pads, spawns, cards, menus) is a no-op when skipped.
- An action does one thing and reads what it needs from the payload or its own properties; no action knows the step it sits in.
- Moving players, opening a way between rooms, raising a tag event: the room's `AGeoTeleporter` pads (`GeoSetTeleporterOpenAction`), and a StateTree task or Blueprint for an event (`../CLAUDE.md`, event bus). No action for them until a step needs one.
- A new gameplay verb (lock an ability, grant loot, open a menu page, say a Cat line) is an action in this folder, written with the feature that needs it, with its own doc row here.
