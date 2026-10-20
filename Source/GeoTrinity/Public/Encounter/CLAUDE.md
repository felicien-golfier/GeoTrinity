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
| `GeoEncounterSubsystem.h` | World subsystem: tag event bus (`Raise(Tag, Payload)`, Blueprint-callable); `bMovementLocked`, the encounters' one movement lock |
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
- Counters: what one rule needs to know about another (falls before the reach, colors visited, "enemy dead AND ally full"). Written by `GeoAddToCounterAction`, read by `GeoCompareCondition` (source `Counter`), watched by `GeoCounterTrigger`. `AddToCounter` / `GetCounter` are Blueprint-callable on the encounter.
- `GeoCompareCondition`: source (`PayloadCount`, `PayloadValue`, `Counter` + name), operator (`==`, `!=`, `<`, `<=`, `>`, `>=`, `MultipleOf`), operand. Other filters are Blueprint children of `UGeoEncounterCondition`.

## Flow
- `Start()` (or `bStartOnBeginPlay`): enter step 0 when not running. `Restart()`: cancel everything (no `OnExit`), back to step 0, in any state; `GeoStartEncounterAction` calls it.
- Enter step: clear counters, arm every rule's trigger (`OnArm`), then run `OnEnter`: in order, each action awaited, alongside the armed rules; cancelled if the step exits first.
- Trigger `Fire(Payload)` → rule's `Condition` (none = pass) → rule's `Actions` in order, each awaited (`Finish()`) → if `bCompletesStep`, complete the step.
- Complete step: disarm all triggers (`OnDisarm`), cancel running actions (`OnCancel`), run `OnExit` awaited, then enter `StepAfter` (None = next in the list). After the last step: `OnCompleted`, encounter finished.
- `EndPlay` (destroyed, streamed out): pending change dropped, triggers disarmed, running actions cancelled, no `OnExit`; every later step change refused.
- Leaving a step (any way) disarms before it cancels: what a cancel causes (`bKillOnStepExit` deaths) never fires a rule of the step being left.
- `GoToStep(Name)` (only while a step runs) is Blueprint-callable for an action that picks the destination itself; the current step's name re-enters it (fresh triggers, counters at 0).
- `bRepeat` false: the rule fires once per visit of the step, then its trigger is disarmed. True: every passing firing runs the actions.
- A rule ignores firings while its own actions are still running; no queue, no overlapping runs of one instance.

## Jump start
Start a chain of encounters at a later one, as if the earlier ones had been played (tutorial: `AI/Tutorial/Flow.md`, "Jump to a room").
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
- An action that waits (`bLatent`) must call `Finish()` and release its timers/bindings in `OnCancel`. Non-latent: `Finish()` is called by the base after `Execute` returns.
- Events a trigger waits for come from the event that happens (a delegate on its hub), never from a call planted in someone else's code (`AI/CodingStyle.md`, "GAS conventions"). No dedicated delegate to bind → raise a tag event.
- Event tags `Encounter.Event.*` are authored in `Config/Tags/GeoGameplayTags.ini`; no C++ names them, only the emitter and the `GeoEventTrigger` that listens.
- `IsDataValid` (editor) on the encounter: unique non-empty step names, `StepAfter` exists, no null trigger or action, no rule without actions and without `bCompletesStep`, a counter compare with no name.
- Runtime misses (nothing to teleport to, a watched actor gone) log to `LogGeoEncounter`; step entries and firings log at `Verbose`.

## Arena
`AGeoArena` stays its own system: a replicated fight lifecycle (match state, barrier, boss bar, wipe reset, rewards, leaderboard) shared by every machine, where an encounter is authority-only scripting. An encounter talks to an arena only through triggers and actions (a death, a barrier); the arena never knows encounters.
