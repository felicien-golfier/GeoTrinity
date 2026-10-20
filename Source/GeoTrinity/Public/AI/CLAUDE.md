# AI

Enemy AI: `AGeoEnemyAIController` + GameplayStateTree.

| File / folder | Role |
|---|---|
| `GeoEnemyAIController.h` | Owns `UStateTreeAIComponent` + `UGeoAIBlackboardComponent`; starts the tree on `OnPossess`; `TriggerAggro` starts the fight |
| `GeoAIBlackboardComponent.h` | Persistent cross-state data (`FGeoAIBlackboardData`) |
| `StateTree/` | Tasks and property functions → `StateTree/CLAUDE.md` |

## Controller
- `GetCurrentTarget()` recomputed every `Tick`: nearest live player; a challenger must stay nearest `TargetSwitchDelay` s to replace it. Tasks read it, never resolve their own.
- `bSetControlRotationFromPawnOrientation = false`; facing goes through `AGeoCharacter::SetTargetYaw`.
- `TriggerAggro`: for boss arenas calls `Arena->StartFight()` then `GameMode->StartMatch()` (gated by `IsBoss()`); non-match bosses get `AI.Boss.AggroEvent`.
- The tree starts only from `InitializeStateTree` (`bStartLogicAutomatically` off). No `StateTree` on the pawn: possession starts no logic. `CanBeAggroed()` false (hidden caster): no aggro watch.
- Server only; cosmetics reach clients through replicated cues or state.
