# AI/StateTree/Ability

StateTree tasks that activate gameplay abilities on the enemy.

## Files
### `STTask_FireAbility`
Activates an ability by `GameplayTag` on the enemy's ASC.
- `FInstanceDataType`: `AbilityTag` (input), `AbilityEndedDelegateHandle` (cleanup)
- `EnterState` — finds ASC, activates ability, binds to ability-ended delegate for async completion
- `ExitState` — unbinds delegate
