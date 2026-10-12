# AI/StateTree/Ability

`STTask_FireAbility` — activates an ability by `AbilityTag` on the enemy ASC, finishes when the ability ends.

- Instance data: `AbilityTag` (input), `AbilityEndedDelegateHandle`.
- `EnterState` activates (no event data → server-driven path) and binds the ended delegate; `ExitState` unbinds.
