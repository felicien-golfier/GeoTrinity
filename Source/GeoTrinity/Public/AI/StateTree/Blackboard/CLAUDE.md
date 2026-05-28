# AI/StateTree/Blackboard

StateTree tasks that read or write `FGeoAIBlackboardData` on `UGeoAIBlackboardComponent`.

> **Pattern for durable cross-state data**: store values in `FGeoAIBlackboardData` (inside `UGeoAIBlackboardComponent`). Link via `TStateTreeExternalDataHandle<UGeoAIBlackboardComponent>` + `Linker.LinkExternalData(BlackboardHandle)`. The schema resolves `UActorComponent` subclasses automatically — no custom schema needed. Never store mutable cross-state data in `FInstanceDataType` (resets on state re-entry). To add a new BB field: add it to `FGeoAIBlackboardData` + add the corresponding op struct in `FSTTask_SetBlackboardInstanceData`.

## Files
### `FSTTask_UpdateBlackboard`
Writes selected fields of `FGeoAIBlackboardData` from the editor. Each field has a typed op struct (`FGeoBlackboardIntFieldOp` / `FGeoBlackboardFloatFieldOp`) with an `EGeoBlackboardOp` enum (None / Set / Add / Multiply). Set Op to anything other than None to activate a field.
- Use on any state to reset or override blackboard values (e.g. reset `CycleCount` when a new phase begins)
