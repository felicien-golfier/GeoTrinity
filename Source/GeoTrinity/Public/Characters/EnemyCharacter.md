# EnemyCharacter.h — AI-controlled character

Extends `AGeoCharacter`. Creates its own ASC directly on the character (not on PlayerState). ASC uses minimal replication mode.

## GAS init
`InitGAS()` — creates `UGeoAbilitySystemComponent` directly on self, calls `InitAbilityActorInfo(Self, Self)`. No PlayerState involved.

## Firing points
- `FiringPoints` (`TArray<AActor*>`) — world actors tagged `"Path"`, collected on `BeginPlay`
- `GetAndAdvanceNextFiringPointLocation(OutLocation)` — round-robin; returns false if array is empty
- Used by `FSTTask_SelectNextFiringPoint` to set the StateTree `TargetLocation` output

## AI
- `StateTree` (`UStateTree*`) — the StateTree asset assigned in BP; handed to `UStateTreeAIComponent` on `BeginPlay`
- `AGeoEnemyAIController` starts the tree on `OnPossess`

## Health reset
- `ResetToFullLifeWhenReachingZero` — if true, health is restored to max instead of destroying the actor at zero health (useful for infinite-respawn enemies)
- `OnHealthChanged(float)` — `BlueprintNativeEvent`; override in BP for death VFX, loot, etc.
