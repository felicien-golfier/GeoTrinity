# Encounter/Action/Cat

Actions giving the placed Cat (`AGeoCat`, `Actor/CLAUDE.md` "GeoCat") one order each. The encounter is the navigator: every point, speed, area and montage lives on these actions, never on the cat.

| File | Order | Waits |
|---|---|---|
| `GeoCatAction.h` | Abstract base: `Cat` (level reference), `bWaitForEnd`; `GiveOrder` (per child), `EndsOnItsOwn` | - |
| `GeoCatWalkAction.h` | `Walk`: `Path` (level actors, read at execution), `Speed`, `StopDistance` (ends short of the last point, facing it) | until the walk completes |
| `GeoCatRoamAction.h` | `Roam`: `Settings` (`FGeoCatRoam`: `Area`, `Margin`, `Pause`, `Speed`) | never: finishes at once (`bWaitForEnd` false by default, ignored) |
| `GeoCatMontageAction.h` | `PlayMontage`: `Montage` | until the montage starts blending out |

- `bWaitForEnd` true: `Finish()` on the cat's `OnOrderEnded` (completed or replaced); false: right after the order, so the next action (a Cat line) runs while the cat moves. An order that ended at once (no montage, empty path) finishes at once.
- A new verb = a child overriding `GiveOrder` (and `EndsOnItsOwn` when it only ends replaced); the wait / cancel logic stays in the base.
- Composition, not special verbs: a clip that ends out of sight is a `GeoCatMontageAction` followed by `GeoSetActorHiddenAction` (the hide lands at blend-out start, before the pose returns); an entrance is a show, then a montage.
- Something running beside a line list (the cat's exit during the last line) goes in its own rule, fired by a counter from `OnEnter`: a rule's actions run alongside `OnEnter` (`../../CLAUDE.md`, "Flow"); the step completes on a counter both lists add to.
- `OnCancel`: unbinds; the cat keeps its order on a normal step exit (it outlives the step, like a line on screen). On abort (`IsAborting()`) the cat's order is dropped (it stops where it stands); its visibility belongs to `GeoSetActorHiddenAction`'s own cancel.
- No `OnSkipped`: the cat is placed hidden, so a skipped room leaves it hidden.
- `IsDataValid` (editor): null `Cat`; walk with an empty path or a null point; montage with none; roam with no `Area`; warning on `bWaitForEnd` for an order that never ends.
