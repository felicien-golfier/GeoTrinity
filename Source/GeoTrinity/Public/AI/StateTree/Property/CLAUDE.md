# AI/StateTree/Property

StateTree property functions. Extend `FStateTreePropertyFunctionCommonBase`. They run each frame to compute a value and write it to an output property, which can then be bound to any condition or task input.

## Files
### `FSTGetHealthRatioPropertyFunction`
Reads the controlled pawn's current health ratio from `UGeoAttributeSetBase`.
- `Input` — `AAIController*` (bind to the owning controller context)
- `Output` — `float` in [0, 1]; outputs 0 if the controller, pawn, or ASC is absent

### `FSTGetBlackboardPropertyFunction`
Reads all fields of `FGeoAIBlackboardData` from `UGeoAIBlackboardComponent` on the controller.
- `Input` — `AAIController*` (bind to the owning controller context)
- `Output` — `FGeoAIBlackboardData Blackboard`; bind any sub-field (e.g. `Blackboard.CycleCount`) to conditions or task inputs
- Adding a new BB field to `FGeoAIBlackboardData` automatically exposes it here — no changes needed to this function
