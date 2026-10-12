# AI/StateTree/Blackboard

`STTask_UpdateBlackboard` — writes selected `FGeoAIBlackboardData` fields.

- Each field has a typed op struct (`FGeoBlackboardIntFieldOp` / `FloatFieldOp`) with `EGeoBlackboardOp` (None / Set / Add / Multiply); None = untouched.
- New blackboard field = field in `FGeoAIBlackboardData` + its op struct here; `STPropertyFunction_GetBlackboard` exposes it automatically.
- Link with `TStateTreeExternalDataHandle<UGeoAIBlackboardComponent>` + `Linker.LinkExternalData`.
