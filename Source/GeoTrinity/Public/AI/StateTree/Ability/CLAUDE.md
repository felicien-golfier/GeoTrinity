# AI/StateTree/Ability

`STTask_FireAbility` — activates an ability by `AbilityTag` on the enemy ASC, finishes when the ability ends.

- Instance data: `AbilityTag` (input), `PatternData` (input, optional), `AbilityEndedDelegateHandle`.
- `PatternData` set: the ability activates with `FPatternAbilityParams` (the caster's arena tag + that data) as event data, like an encounter firing (`Abilities/Base/CLAUDE.md`, "Activation params"); the miniboss's wave sets `bRandomDirection` this way. Unset: by tag alone.
- `STTask_StrikeInRange`: ticks while running; once the current target (`GetCurrentTarget()`) is within `Range` (2D) it activates `AbilityTag` once and Succeeds when that ability ends. Never fires out of range. The Training Room Cat's kill (`Characters/CLAUDE.md`, "GeoTrainingCat").
- `EnterState` activates (no event data → server-driven path) and binds the ended delegate; `ExitState` unbinds.
