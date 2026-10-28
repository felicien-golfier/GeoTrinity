# Encounter

Scripted sequences of steps driven by triggers and actions, authored in the level and written in Blueprint. First user: the tutorial.

| File / folder | Role |
|---|---|
| `GeoEncounter.h` | `AGeoEncounter`: placed actor holding `Steps`; runs the current step, arms its rules, moves between steps, holds the step counters |
| `GeoEncounterTypes.h` | `FGeoEncounterPayload`, `FGeoEncounterRule`, `FGeoEncounterStep`, `EGeoCompareSource`, `EGeoCompareOperator` |
| `GeoEncounterObject.h` | `UGeoEncounterObject`: abstract base of trigger, condition, action; `GetWorld()` (null on the CDO), `GetEncounter()` (typed outer) |
| `GeoEncounterTrigger.h` | `UGeoEncounterTrigger`: base of every trigger; binds in `OnArm`, unbinds in `OnDisarm`, calls `Fire(Payload)` |
| `GeoEncounterCondition.h` | `UGeoEncounterCondition`: `Evaluate(Payload) → bool` filter on a rule |
| `GeoCompareCondition.h` | The stock condition: payload `Count`, payload `Value` or a step counter, compared with an operand |
| `GeoEncounterAction.h` | `UGeoEncounterAction`: base of every action; `Execute(Payload)`, `Finish()`, `OnCancel()`, `bLatent` |
| `GeoEncounterSubsystem.h` | World subsystem: tag event bus (`Raise(Tag, Payload)`, Blueprint-callable); `bMovementLocked`, the encounters' one movement lock; `OnCatLine` (native): every Cat line as it shows, tokens replaced (the tutorial bot's line log); aborts every encounter of its world when the session ends (below); `GetSharedCaster()` / `GrantSharedAbility(Class)`: the encounters' one ability caster (below) |
| `Trigger/` | Stock triggers → `Trigger/CLAUDE.md` |
| `Action/` | Stock actions → `Action/CLAUDE.md` |

## Model
```
AGeoEncounter
 ├─ Counters : TMap<FName, int32>   (step-scoped, cleared on step entry)
 └─ Steps[]   (ordered)
     ├─ Name
     ├─ OnEnter : Actions[]
     ├─ Rules[] : { Trigger, Condition?, Actions[], bRepeat, bCompletesStep, StepAfter }
     └─ OnExit  : Actions[]
```
- Trigger, Condition, Action derive from `UGeoEncounterObject`: `Blueprintable`, `EditInlineNew`, `DefaultToInstanced`. The Details panel picks a class, the class is code (C++ or Blueprint). No enum of trigger types anywhere.
- Instances live on the placed encounter, so properties hold direct level references (pad, volume, arena); characters are never referenced, they are spawned and watched by filter (`Trigger/CLAUDE.md`). A reusable encounter is a Blueprint child of `AGeoEncounter`.
- `FGeoEncounterPayload`: `Actor` (the actor the event is about: who entered, who died), `Count`, `Value`, `Tag`. The trigger fills it; conditions and actions read it. A trigger keeps its own running number (deaths since arm) and sends it as `Count`.
- Counters: what one rule needs to know about another (falls before the reach, colors visited, "enemy dead AND ally full"). Written by `GeoAddToCounterAction`, read by `GeoCompareCondition` (source `Counter`), watched by `GeoCounterTrigger`. `AddToEncounterCounter` / `GetCounter` are Blueprint-callable on the encounter.
- `GeoCompareCondition`: source (`PayloadCount`, `PayloadValue`, `Counter` + name), operator (`==`, `!=`, `<`, `<=`, `>`, `>=`, `MultipleOf`), operand. Other filters are Blueprint children of `UGeoEncounterCondition`.

## Flow
- `Start()` (or `bStartOnBeginPlay`): enter step 0 when not running. `Restart()`: cancel everything (no `OnExit`), back to step 0, in any state. `GeoStartEncounterAction` calls `StartAfterSkippingChain()` (Arrival reset).
- Enter step: clear counters, arm every rule's trigger (`OnArm`), then run `OnEnter`: in order, each action awaited, alongside the armed rules; cancelled if the step exits first.
- Trigger `Fire(Payload)` → rule's `Condition` (none = pass) → rule's `Actions` in order, each awaited (`Finish()`) → if `bCompletesStep`, complete the step.
- Complete step: disarm all triggers (`OnDisarm`), cancel running actions (`OnCancel`), run `OnExit` awaited, then enter `StepAfter` (None = next in the list). After the last step: `OnCompleted`, encounter finished.
- `EndPlay` (destroyed, streamed out): pending change dropped, triggers disarmed, running actions cancelled, no `OnExit`; every later step change refused.
- Leaving a step (any way) disarms before it cancels: what a cancel causes (`bKillOnStepExit` deaths) never fires a rule of the step being left.
- `GoToStep(Name)` (only while a step runs) is Blueprint-callable for an action that picks the destination itself; the current step's name re-enters it (fresh triggers, counters at 0).
- `bRepeat` false: the rule fires once per visit of the step, then its trigger is disarmed. True: every passing firing runs the actions.
- A rule ignores firings while its own actions are still running; no queue, no overlapping runs of one instance.
- A `bCompletesStep` rule that fired owns the step until its actions end: every other rule ignores firings meanwhile (a long completing list, e.g. a line that waits for the respawn, never lets a sibling complete the step first, `SafeSpots`: a death must not let the remaining waves open the exit). The step's `OnEnter` keeps running until the completion cancels it.

## Arrival reset
Every chain start (`GeoStartEncounterAction`, `Action/CLAUDE.md`; the jump below) lands the world in the state the target encounter expects, whatever the previous room was doing: a portal never waits for, or lets run on, the room left behind.
- `AGeoEncounter::StartAfterSkippingChain()` order: (1) `Abort()` every other encounter of the world, whatever its state (a completed room can leave a Cat line standing); (2) skip replay of the chain before the target (below); (3) `Restart()` the target (a running one starts over). Nothing about the left room is cleared by the encounter: every action undoes its own leftovers.
- `Abort()` (server, Blueprint-callable): stops the current step without `OnExit` (disarm, then the normal cancel of running and held actions), drops the pending change, then calls `OnCancel` on every action of every step (`OnEnter`, rules, `OnExit`) so the ones that finished at once, or in an earlier step, can undo what they left standing. `IsAborting()` is true during the whole call. Back to `Idle`, no `OnCompleted`, no later step change until `Start()` or `Restart()`.
- `Abort()` is synchronous, also from inside the encounter's own call (the room whose rule runs the start action): the runs are dropped at once (`bActive` false), so the rest of that rule's list and a completion it asked never happen. Synchronous so the left room's undo is over before the arrived room's `OnEnter` runs.
- Action contract: `OnCancel` undoes what the action left standing and is safe on an action that never ran or already ended (a "did something" flag, set in `Execute`, cleared by the cancel). An action whose leftover outlives a normal step exit on purpose (a Cat line finishing, the Cat's order) undoes it only when `IsAborting()`. Per-action effects: `Action/CLAUDE.md`, "Cancel".
- Skip replay then rebuilds what the target needs: abilities unlocked, movement lock / unlock pairs netted (the one lock makes the aborted room's lock harmless: lock while locked does nothing, the replayed unlock frees). A player who took the portal early gets what the skipped part would have given.
- Presentation of a skipped part (lines, pads, spawns, menus) is never replayed (`OnSkipped` no-op); what a room needs of the previous one beyond abilities and locks is set by the target's own `OnEnter`.

## Session end
The world's end is an `Abort()` of every encounter, same synchronous stop as the arrival reset, so nothing advances while the session is torn down.
- Why: the viewport is destroyed before the encounters' `EndPlay` (PIE: `UEditorEngine::EndPlayMap` → `CleanupGameViewport`, before `UWorld::BeginTearingDown`). Destroying the HUD ends the Cat line it shows (`NativeDestruct` → `EndLine`), which finishes the awaited `GeoCatLineAction`; the encounter would then run the next action with no player controller left.
- `UGeoEncounterSubsystem` binds in `Initialize`, unbinds in `Deinitialize`: `FWorldDelegates::OnWorldBeginTearDown` (quit, travel; packaged too) and, editor only, `FWorldDelegates::OnPIEEnded` (fires before the viewport cleanup, matched to the world by its game instance). Both call the same handler.
- Handler: once per subsystem (a flag), `Abort()` every `AGeoEncounter` of the world. After it every encounter is `Idle`: a later `Finish()` is ignored (not awaited), no step change until `Start()` / `Restart()`.
- Actions need nothing of their own: `OnCancel` already undoes what they left (contract above). An action never has to tolerate "no player controller" for this; `GeoSetMovementLockedAction` keeps its ensure for a real missing player.

## Shared caster
Every ability an encounter fires runs on one caster per world, owned by `UGeoEncounterSubsystem`; no room places a caster or an arena for it. Boss fights and real enemies (dummies, adds) keep their own abilities and trees.
- `GetSharedCaster()`: server only (null on a client). First call spawns it: `AHiddenCasterCharacter` through `AEnemyCharacter::SpawnEnemy`, default setup (enemy team, no abilities, no tree), no arena, at the world origin, `ArbitraryCharacterZ`. Lives until the world ends; an abort never resets it (its abilities stop through the actions' `OnCancel`).
- Where it stands does not matter: an ability's origin comes from the room the firing action names (`FPatternAbilityParams`, `AbilitySystem/Abilities/Base/CLAUDE.md`, "Activation params"), its aim from the ability itself, pattern VFX are absolute.
- `GrantSharedAbility(Class)`: the caster's spec of that class, else `GiveAbility` (level 1); kept for the session, so abilities are granted on first use, never listed.
- Refusal, the one place it is checked: a class whose CDO `RequiresArena()` (`AbilitySystem/Abilities/Base/CLAUDE.md`, "RequiresArena") → `ensureMsgf`, invalid handle, nothing granted.
- One spec per class: a second firing of a class still running is refused (logged by the action). One room fires at a time (solo tutorial); rooms firing at once are not supported.

## Jump start
Start a chain of encounters at a later one, as if the earlier ones had been played (tutorial: `AI/Tutorial/Flow.md`, "Jump to a room"). Same mechanism as the arrival reset: the world-URL jump is a reset whose target comes from the portal instead of a start action.
- `AGeoEncounter::JumpStart`: optional level reference to the `APlayerStart` of its room. Target (`FindJumpTarget()`) from the world URL's portal: a number N = the Nth encounter along the chain (0 = the start encounter); else the encounter whose `JumpStart->PlayerStartTag` equals it.
- With a target, no `bStartOnBeginPlay` start; the target's `BeginPlay` moves the first local pawn onto its `JumpStart` (a number matches no tag, so the engine spawned it on the default start; `GameClasses/Match/CLAUDE.md`, "Player starts"), then calls its `StartAfterSkippingChain()`. No target (no portal, no match, number past the chain's end) → normal start.
- Chain: from the `bStartOnBeginPlay` encounter (`FindChainStart()`), next = the `Target` of the first `GeoStartEncounterAction` in its steps (any action list), until the target. Target not on the chain → log, start it with nothing skipped. The walk lives on the encounter: it reads other encounters' steps.
- Skip replay, strict order: encounter by encounter along the chain; per encounter, steps in list order; per step, `OnEnter` actions then `OnExit` actions, each in list order → `OnSkipped()`. All in one call, before the target's `Start()`. Pairs that must cancel out (movement lock / unlock) therefore net to zero.
- Rule actions are never replayed: reactions to events (quips, fail paths). Anything that must hold after a skip (an unlock, a lock / unlock pair) sits in `OnEnter` / `OnExit`.
- `OnSkipped()` (`UGeoEncounterAction`, `BlueprintNativeEvent`): no-op by default; an action overrides it only for a lasting effect on the player (`Action/CLAUDE.md`). Never latent, no `Finish()`.

## Invariants
- Authority only: every step change goes through one server gate (`GeoLib::IsServer`, in the change request that `Start`, `Restart` and `GoToStep` share); clients hold the actor and run nothing; nothing replicates. What a client must see travels by the action's own means (cue, replicated state of the thing it changes), never through the encounter. A solo map still runs as a listen server (`GeoLib::IsServer` is false in Standalone).
- One step change at a time: a completion, `GoToStep` or `Restart` asked while the encounter is inside a call (arming, `Execute`, `Fire`, `OnExit`) is stored and applied once that call returns; the first request wins, later ones are dropped.
- `Finish()` from an action the encounter is not awaiting (cancelled, or from a left step) is ignored.
- State of a trigger or action is reset in `OnArm` / `Execute`, never in a constructor: a restart or a revisited step starts clean.
- A trigger binds in `OnArm` and unbinds in `OnDisarm`; the encounter always calls `OnDisarm` on a step exit, on `Restart()` and on `EndPlay`.
- Run state (`FGeoEncounterActionRun`) is reflected: a payload actor destroyed while its run waits reaches the next action as null.
- A trigger on a state (inside a volume, health past a ratio) fires from `OnArm` when the state already holds; an event trigger (a death, a raised tag) only fires on the next occurrence.
- A firing from `OnArm` may disarm the trigger at once (a rule that does not repeat): such a trigger binds everything before its first `Fire`, and stops arming once disarmed, so `OnDisarm` always undoes all of `OnArm`.
- An action that leaves something running (enemies, a zone) finishes at once and asks `HoldUntilStepLeft`: its `OnCancel` runs when the step is left (asked from `OnExit`: when the step it leads to is left), so a list never waits on it. Run again while held (a repeating rule), it ends its previous run first, as a cancel does.
- An action that waits (`bLatent`) must call `Finish()` and release its timers/bindings in `OnCancel`. Non-latent: `Finish()` is called by the base after `Execute` returns.
- Events a trigger waits for come from the event that happens (a delegate on its hub), never from a call planted in someone else's code (`AI/CodingStyle.md`, "GAS conventions"). No dedicated delegate to bind → raise a tag event.
- Event tags `Encounter.Event.*` are authored in `Config/Tags/GeoGameplayTags.ini`; no C++ names them, only the emitter and the `GeoEventTrigger` that listens.
- `IsDataValid` (editor) on the encounter: unique non-empty step names, `StepAfter` exists, no null trigger or action, no rule without actions and without `bCompletesStep`, a counter compare with no name, an action after a `GeoStartEncounterAction` in its list (the start aborts the encounter it sits in, so what follows never runs: Arrival reset).
- Runtime misses (nothing to teleport to, a watched actor gone) log to `LogGeoEncounter`; step entries and firings log at `Verbose`.

## Arena
`AGeoArena` stays its own system: a replicated fight lifecycle (match state, barrier, boss bar, wipe reset, rewards, leaderboard) shared by every machine, where an encounter is authority-only scripting. An encounter talks to an arena only through triggers and actions (a death, a barrier); the arena never knows encounters. An encounter fires no ability through an arena: the shared caster (above).
