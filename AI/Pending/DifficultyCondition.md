# DifficultyCondition — pending

Goal: one StateTree per enemy for every difficulty: a state is entered only at chosen difficulties.
Priority: 3
Docs written: none yet (design open, workflow steps 2-3 when picked up). Owner doc: `Source/GeoTrinity/Public/AI/StateTree/CLAUDE.md`.

## Idea
- `STCondition_Difficulty` (`FStateTreeConditionCommonBase`, the project has none yet): an `EGeoDifficulty` bitmask (`Tool/GeoDifficulty.h`), reads the owner's combat level (the difficulty bit, `AGeoArena` stamps it), used as a state enter condition. Alternative: the same mask on `STTask_FireAbility`.
- Why: a tree such as the ally's or a boss's can drop a state on Safe without a second tree or a setup flag (`bSafeDropsStateTree` is gone).

## Read map
- `Source/GeoTrinity/Public/Tool/GeoDifficulty.h`, `Source/GeoTrinity/Public/Tool/CLAUDE.md` ("Difficulty").
- `Source/GeoTrinity/Public/AI/StateTree/CLAUDE.md`, `Source/GeoTrinity/Public/AI/StateTree/Ability/STTask_FireAbility.h`, `Source/GeoTrinity/Private/AI/StateTree/Ability/STTask_FireAbility.cpp`.
- `AI/MCP/MCP_StateTree.md` — StateTree builder (`GeoStateTreeBuilderUtil` in `Source/GeoTrinityEditor`) — how a condition is added from a script.
