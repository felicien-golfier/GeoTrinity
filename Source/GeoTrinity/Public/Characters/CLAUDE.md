# Characters

```
AGeoCharacter            ASC interface + team agent
├── APlayableCharacter   ASC on PlayerState, Enhanced Input, runtime class switch
└── AEnemyCharacter      own ASC (minimal replication), StateTree AI
```

| File | Role |
|---|---|
| `GeoCharacter.h` | Abstract base: ASC, input, movement, health bar, game feel, turn-rate clamp |
| `PlayableCharacter.h` | Human player: input forwarding, `ChangeClass()`, gauges, downed/revive |
| `EnemyCharacter.h` | AI enemy: own ASC, StateTree, boss death / reset |
| `PlayerClassTypes.h` | `EPlayerClass`: Triangle=1, Circle=2, Square=3, All=4 |
| `PlayerClassDataAsset.h` | `UPlayerClassDataAsset`: `TMap<EPlayerClass, FPlayerClassData>`; read via `GameDataSettings::PlayerClassData` |
| `Component/` | Character components → `Component/CLAUDE.md` |

## GeoCharacter
- Components always present: `UGeoAbilitySystemComponent`, `UGeoInputComponent`, `UGeoCharacterMovementComponent`, `WidgetAnchorComponent`, `CharacterWidgetComponent`, `UGeoGameFeelComponent`.
- `InitGAS()`: subclasses override and call `InitAbilityActorInfo`; from `PossessedBy` / `OnRep_PlayerState`.
- Facing: always `SetTargetYaw(float)`; `Tick` closes the gap at `MaxRotationSpeed` (720 deg/s). Callers: `UpdateAimRotation`, `STTask_ChaseTarget`, `STTask_FaceTarget`, `UGeoAITask_MoveTo::TickTask`.
- World widgets attach to `WidgetAnchorComponent` (non-rotating), never the root.
- Health bar: C++ default subobject from `GameDataSettings` soft classes (`CombattantWidgetComponentClass`, `DefaultCharacterHealthBarWidgetClass`); skipped on dedicated server; bind/visibility through `IGeoCombattantWidgetHost`.
- `SetCombattantWidgetVisible(false)` only hides the boss's floating bar while the screen bar shows; condition `IsPlayerControlled() && IsLocallyControlled()` for the host's own.
- `TeamId` (`ETeam`) for attitude queries.

## PlayableCharacter
- ASC lives on `AGeoPlayerState`; `InitGAS` fetches it and calls `InitAbilityActorInfo(PlayerState, Self)`.
- `ChangeClass(EPlayerClass)`: swaps mesh/anim/abilities, ends with `GiveLife()`.
- `ApplyClassData(EPlayerClass)`: visuals only (mesh, anim, `AliveMaterial`/`DeathMaterial`), safe on every client via `AGeoPlayerState::ApplyClassDataToPawn`.
- `GetClassData()` resolves through `GameDataSettings::PlayerClassData`.
- `GiveLife()` (server): applies class `DefaultAttributes`, `ReactivatePassiveAbilities()`; called at end of `ChangeClass` and `ReviveLogic`. Leaving a class ends its passives via `ClearPlayerClassAbilities`.
- Input: `GeoInputComponent::BindAbilityActions`; `AbilityInputTagHeld` per frame.
- Gauges: `ShowDeployChargeGauge(Ability)` / `HideDeployChargeGauge()` through `IGeoChargeGaugeWidgetInterface`; widget components `Space = Screen`.
- Body material swaps go through `SetBodyMaterial` (re-initialises MIDs).

## Death / revive
- `Death()` (server) sets replicated `bIsDead`, `StopCharacter()`, then `AGeoGameState::NotifyPlayerDied`. Policy = GameState's (`GameClasses/Match/CLAUDE.md`).
- `OnRep_IsDead` runs the same `DeathLogic()` / `ReviveLogic()`; `RemoveActiveEffects` is server-guarded inside both.
- Death keeps the gems: `DeathLogic()` → `ASC->HandleOwnerDeath(GemComponent->GetSurvivingEffectHandles())`. Gem stats GE stays (a dead player's deployables, zones and passives still read their stats); every other GE goes. Each ability's `OnOwnerDeath()` decides if it ends (`Abilities/Base/CLAUDE.md`); Core passives stay active.
- `Revive()`: `ResetAbilitiesAndEffects()` (full reset: cancel every ability but the uncancellable Core passives, remove every GE), `StopAllSpawnedElements()`, re-apply class visuals + `OnRevived`, `GiveLife()` (`ApplyGems` re-grants stats + Core passives), `RestartCharacter()`. `ChangeClass` uses the same full reset.
- Late joiners spawn alive and are not in `FightPlayers`.

## EnemyCharacter
- `InitGAS()` creates the ASC on self; `StateTree` asset handed to `UStateTreeAIComponent`; `AGeoEnemyAIController` starts it on `OnPossess`.
- `ResetToFullLifeWhenReachingZero` = training dummy (restores health instead of `OnEnemyDefeated`).
- `OnHealthChanged(float)` BlueprintNativeEvent for death VFX / loot; `ResetForNewAttempt`.
- Firing points live in `STTask_SelectNextFiringPoint` instance data.

## Enemy Blueprint defaults
- A default every enemy shares (base `StateTree`, …) is set once on the CDO of `BP_BaseEnemyBlueprintToInheritFrom`; boss Blueprints hold only their own values (`BehaviourStateTree`, …).
- Moving a default up: write the base, recompile it, then recompile and re-save the children so identical overrides drop. A parent default also reaches dummies and tutorial enemies: check the children first.
