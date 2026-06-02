# EnemyCharacter.h — AI-controlled character

Extends `AGeoCharacter`. Creates its own ASC directly on the character (not on PlayerState). ASC uses minimal replication mode.

## GAS init
`InitGAS()` — creates `UGeoAbilitySystemComponent` directly on self, calls `InitAbilityActorInfo(Self, Self)`. No PlayerState involved.

## Firing points
- Firing point collection and round-robin selection live entirely in `FSTTask_SelectNextFiringPoint`'s instance data — EnemyCharacter has no `FiringPoints` property of its own

## AI
- `StateTree` (`UStateTree*`) — the StateTree asset assigned in BP; handed to `UStateTreeAIComponent` on `BeginPlay`
- `AGeoEnemyAIController` starts the tree on `OnPossess`

## Health reset
- `ResetToFullLifeWhenReachingZero` — if true, health is restored to max instead of destroying the actor at zero health (useful for infinite-respawn enemies)
- `OnHealthChanged(float)` — `BlueprintNativeEvent`; override in BP for death VFX, loot, etc.
