# AI/StateTree/Property

Property functions (`FStateTreePropertyFunctionCommonBase`): computed each frame into an output bound to conditions or task inputs.

| Function | Input → Output |
|---|---|
| `STPropertyFunction_GetHealthRatio` | `AAIController*` → `float` [0, 1] (0 when controller / pawn / ASC missing) |
| `STPropertyFunction_GetBlackboard` | `AAIController*` → whole `FGeoAIBlackboardData`; bind sub-fields (e.g. `Blackboard.CycleCount`) |
