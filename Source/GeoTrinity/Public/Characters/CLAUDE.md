# Characters

All playable and enemy character classes plus their components.

## Hierarchy
```
AGeoCharacter  (base — IAbilitySystemInterface, IGenericTeamAgentInterface)
├── APlayableCharacter  (ASC on PlayerState, Enhanced Input, runtime class-switching)
└── AEnemyCharacter     (own ASC, StateTree AI, round-robin firing points)
```

## Files
| File | Role |
|---|---|
| `GeoCharacter.h` | Abstract base — ASC, input, movement, health bar, game feel |
| `PlayableCharacter.h` | Human player — input forwarding, `ChangeClass()`, deploy gauge |
| `EnemyCharacter.h` | AI enemy — own ASC, `FiringPoints`, `GetAndAdvanceNextFiringPointLocation()` |
| `PlayerClassTypes.h` | `EPlayerClass` enum: `Triangle=1`, `Circle=2`, `Square=3`, `All=4` |

## Component Subfolder
| Component | Role |
|---|---|
| `GeoCharacterMovementComponent` | Cached base speed, `ApplySpeedMultiplier(float)` |
| `GeoDeployableManagerComponent` | Tracks deployables, enforces max count, `RecallAll()` |
| `GeoGameFeelComponent` | Hit flash, recoil spring, cue rate-limiting |
| `ShieldBurstPassiveComponent` | Square passive gauge — replicated `GaugeRatio`, `SetGaugeRatio()` |

See `Component/CLAUDE.md` for full details.
