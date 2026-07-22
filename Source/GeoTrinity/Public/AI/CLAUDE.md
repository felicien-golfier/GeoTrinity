# AI

Enemy AI system using `AGeoEnemyAIController` with the GameplayStateTree plugin.

## Files
| File | Role |
|---|---|
| `GeoEnemyAIController.h` | AI controller; owns `UStateTreeAIComponent` and `UGeoAIBlackboardComponent` (`GeoBlackBoard`), starts tree on `OnPossess` |
| `GeoAIBlackboardComponent.h` | Persistent AI state (cross-state blackboard); linked by tasks via `TStateTreeExternalDataHandle<UGeoAIBlackboardComponent>` |
| `StateTree/Ability/` | Fire ability tasks |
| `StateTree/Blackboard/` | SetBlackboard task |
| `StateTree/Movement/` | MoveTo, SelectNextFiringPoint (draws from the boss's own arena via `GetArenaOfBoss`, so a second boss can't path to another room's points) |
| `StateTree/Property/` | Property functions (GetHealthRatio, GetBlackboard) |
| `StateTree/Utility/` | SendEventAfterNCycles |

See `StateTree/CLAUDE.md` for task authoring patterns.
