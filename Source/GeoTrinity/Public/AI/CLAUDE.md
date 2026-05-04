# AI

Enemy AI system using `AGeoEnemyAIController` with the GameplayStateTree plugin.

## Files
| File | Role |
|---|---|
| `GeoEnemyAIController.h` | AI controller; owns `UStateTreeAIComponent`, starts tree on `OnPossess` |
| `StateTree/` | StateTree task structs — preferred, add new tasks here |
| `Tasks/` | Legacy Behavior Tree tasks (`UBTTask_*`) — do not add new tasks here |

## StateTree vs BehaviorTree
New AI logic goes in `StateTree/`. The `Tasks/` folder contains legacy BT tasks kept for reference.

See `StateTree/CLAUDE.md` for task authoring patterns.
