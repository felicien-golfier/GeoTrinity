# AI

Enemy AI system using `AGeoEnemyAIController` with the GameplayStateTree plugin.

## Files
| File | Role |
|---|---|
| `GeoEnemyAIController.h` | AI controller; owns `UStateTreeAIComponent` and `UGeoAIBlackboardComponent` (`GeoBlackBoard`), starts tree on `OnPossess`. Recomputes `GetCurrentTarget()` every `Tick` (nearest live player; a different player must stay nearest for `TargetSwitchDelay` seconds before it replaces the current target) — `STTask_ChaseTarget`/`STTask_FaceTarget` read it instead of each resolving a target |
| `GeoAIBlackboardComponent.h` | Persistent AI state (cross-state blackboard); linked by tasks via `TStateTreeExternalDataHandle<UGeoAIBlackboardComponent>` |
| `StateTree/Ability/` | Fire ability tasks |
| `StateTree/Blackboard/` | SetBlackboard task |
| `StateTree/Movement/` | MoveTo, SelectNextFiringPoint (draws from the boss's own arena via `GetArenaOfBoss`, so a second boss can't path to another room's points) |
| `StateTree/Property/` | Property functions (GetHealthRatio, GetBlackboard) |
| `StateTree/Utility/` | SendEventAfterNCycles |

See `StateTree/CLAUDE.md` for task authoring patterns.
