# Encounter

Scripted sequences of steps driven by triggers and actions, authored in the level and written in Blueprint. First user: the tutorial; meant for any scripted event (boss phases later).

| File / folder | Role |
|---|---|
| `GeoEncounter.h` | `AGeoEncounter`: placed actor holding `Steps`; runs the current step, arms its rules, moves between steps |
| `GeoEncounterTypes.h` | `FGeoEncounterPayload`, `FGeoEncounterRule`, `FGeoEncounterStep` |
| `GeoEncounterTrigger.h` | `UGeoEncounterTrigger`: base of every trigger; binds in `OnArm`, calls `Fire(Payload)` |
| `GeoEncounterCondition.h` | `UGeoEncounterCondition`: `Evaluate(Payload) → bool` filter on a rule |
| `GeoEncounterAction.h` | `UGeoEncounterAction`: base of every action; `Execute(Payload)`, `Finish()` |
| `GeoEncounterSubsystem.h` | World subsystem: tag event bus (`Raise(Tag, Payload)`) |
| `Trigger/` | Stock triggers → `Trigger/CLAUDE.md` |
| `Action/` | Stock actions → `Action/CLAUDE.md` |

## Model
```
AGeoEncounter
 └─ Steps[]   (ordered)
     ├─ Name
     ├─ OnEnter : Actions[]
     ├─ Rules[] : { Trigger, Condition?, Actions[], bRepeat, bCompletesStep, StepAfter }
     └─ OnExit  : Actions[]
```
- Trigger, Condition, Action are `Blueprintable`, `EditInlineNew`, `DefaultToInstanced` UObjects: the Details panel picks a class, the class is code (C++ or Blueprint). No enum of trigger types anywhere.
- Instances live on the placed encounter, so properties hold direct level references (gate, dummy, volume). A reusable encounter is a Blueprint child of `AGeoEncounter`.
- `FGeoEncounterPayload`: `Instigator` (actor), `Count`, `Value`, `Tag`. The trigger fills it; conditions and actions read it. A trigger keeps its own running numbers (death count) in its own variables and sends them as `Count`.

## Flow
- `Start()` (or `bStartOnBeginPlay`): enter step 0. `Restart()`: cancel everything, back to step 0.
- Enter step: run `OnEnter` (not awaited), arm every rule's trigger (`OnArm`).
- Trigger `Fire(Payload)` → rule's `Condition` (none = pass) → rule's `Actions` run in order, each awaited (`Finish()`) → if `bCompletesStep`, complete the step.
- Complete step: disarm all triggers (`OnDisarm`), cancel running actions (`OnCancel`), run `OnExit` awaited, then enter `StepAfter` (None = next in the list). After the last step: `OnCompleted`, encounter finished.
- `GoToStep(Name)` is Blueprint-callable for an action that picks the destination itself.
- `bRepeat` false: the rule fires once per visit of the step, then its trigger is disarmed. True: every passing firing runs the actions.
- A rule ignores firings while its own actions are still running; no queue, no overlapping runs of one instance.

## Invariants
- Server only. Clients hold the actor and run nothing (`HasAuthority()` gate in `BeginPlay`); nothing replicates. What a client must see travels by the action's own means (cue, replicated state of the thing it changes), never through the encounter.
- State of a trigger or action is reset in `OnArm` / `Execute`, never in a constructor: a restart or a revisited step starts clean.
- A trigger binds in `OnArm` and unbinds in `OnDisarm`; the encounter always calls `OnDisarm` on a step exit and on `Restart()`.
- An action that waits (`bLatent`) must call `Finish()` and release its timers/bindings in `OnCancel`. Non-latent: `Finish()` is called by the base after `Execute` returns.
- Events a trigger waits for come from the event that happens (a delegate on its hub), never from a call planted in someone else's code (`AI/CodingStyle.md`, "GAS conventions"). No dedicated delegate to bind → raise a tag event.
- Event tags `Encounter.Event.*` are authored in `Config/Tags/GeoGameplayTags.ini`; no C++ names them, only the emitter and the `EventTrigger` that listens.
- `IsDataValid` on the encounter: unique non-empty step names, `StepAfter` exists, no null trigger or action, no rule without actions and without `bCompletesStep`.
- `Geo.Encounter.Debug` (non-shipping) draws each running encounter: step name, armed triggers, last payload fired.
