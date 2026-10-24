# Characters

```
AGeoCharacter            ASC interface + team agent
├── APlayableCharacter   ASC on PlayerState, Enhanced Input, runtime class switch
└── AEnemyCharacter      own ASC (minimal replication), StateTree AI
    ├── AHiddenCasterCharacter   invisible, untouchable ability caster for scripted rooms
    └── AGeoSummonedDummy        summoned add: growing damage zone at its feet, turns ally on death
```

| File | Role |
|---|---|
| `GeoCharacter.h` | Abstract base: ASC, input, movement, health bar, game feel, turn-rate clamp |
| `PlayableCharacter.h` | Human player: input forwarding, `ChangeClass()`, gauges, downed/revive |
| `EnemyCharacter.h` | AI enemy: own ASC, StateTree, boss death / reset |
| `GeoEnemySetup.h` | `FGeoEnemySetup`: per-spawn variant of an enemy class (team, dies or resets, StateTree, extra abilities, health, actor tag) |
| `HiddenCasterCharacter.h` | Enemy with nothing on screen and nothing to hit: casts abilities for an encounter in a room with no boss |
| `GeoSummonedDummy.h` | Add summoned by `GeoSummonAddAbility` (tutorial miniboss): its zone grows until it dies, then an ally takes its place |
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
- `Death()` (server) sets replicated `bIsDead`, broadcasts `AGeoGameState::OnCharacterDied` (every character, not only players), runs `DeathLogic()`: `StopCharacter()`, then `AGeoGameState::NotifyPlayerDied`. Policy = GameState's (`GameClasses/Match/CLAUDE.md`).
- `OnRep_IsDead` runs the same `DeathLogic()` / `ReviveLogic()`; `RemoveActiveEffects` is server-guarded inside both.
- Death keeps the gems: `DeathLogic()` → `ASC->HandleOwnerDeath(GemComponent->GetSurvivingEffectHandles())`. Gem stats GE stays (a dead player's deployables, zones and passives still read their stats); every other GE goes. Each ability's `OnOwnerDeath()` decides if it ends (`Abilities/Base/CLAUDE.md`); Core passives stay active.
- `Revive()`: `ResetAbilitiesAndEffects()` (full reset: cancel every ability but the uncancellable Core passives, remove every GE), `StopAllSpawnedElements()`, re-apply class visuals + `OnRevived`, `GiveLife()` (`ApplyGems` re-grants stats + Core passives), `RestartCharacter()`. `ChangeClass` uses the same full reset.
- Late joiners spawn alive and are not in `FightPlayers`.
- Void: the world's `KillZ` (`World/CLAUDE.md`) → engine `FellOutOfWorld` (server only, from character movement) → overridden to `Death()`, never the engine's destroy. Already dead: nothing (the body may keep falling until the respawn teleports it). Any character, player or enemy; a dash (root motion, Z held) crosses a gap and falls only if it ends over the void.

## EnemyCharacter
- `InitGAS()` creates the ASC on self; `StateTree` asset handed to `UStateTreeAIComponent`; `AGeoEnemyAIController` starts it on `OnPossess`.
- `AggroRadiusOverride` (from `FGeoEnemySetup::AggroRadius`, negative = none) beats the controller's `AggroRadius`.
- `ResetToFullLifeWhenReachingZero` = training dummy (restores health instead of `OnEnemyDefeated`).
- `OnHealthChanged(float)` BlueprintNativeEvent for death VFX / loot; `ResetForNewAttempt`.
- Firing points live in `STTask_SelectNextFiringPoint` instance data.

## Enemy setup
- Purpose: a scripted room's enemies (dummy, chaser, shooter, hurt ally) are one Blueprint (`BP_Dummy`: look only) plus an `FGeoEnemySetup` written where they are spawned, never a Blueprint per variant.
- Fields: `Team`, `bResetToFullLife` (default false: dies at zero), `StateTree` (null = the class's), `BehaviourStateTree` (null = the class's; a boss's spell chain inside the base tree's fight-start gate), `AbilityTags` (added to the ASC's `StartupAbilityTags`), `MaxHealth` (`FScalableFloat`, 0 = the class's `DefaultAttributes`; reads `CT_Tutorial` rows), `StartHealthRatio` (0..1, default 1), `Tag` (actor tag, None = none; how triggers find a spawned one), `AggroRadius` (cm, negative = the controller's own `AggroRadius`; the tutorial sets 100: an enemy there wakes on a hit or at point-blank, never from afar).
- `AggroRadius` is kept on the enemy by `ApplySetup`; its controller reads it when the aggro watch starts (`AI/CLAUDE.md`).
- `AEnemyCharacter::SpawnEnemy(World, Class, Location, Owner, Arena*, Setup*)` (static, server): the one spawn of an enemy, arena bosses included: `ArbitraryCharacterZ`, unit scale, deferred spawn, `Arena` (null = none) and `ApplySetup` (when a setup is given) before `FinishSpawning`. Both run before `FinishSpawning`, so the controller's `OnPossess` and `InitGAS` (BeginPlay) already see the StateTree, team and abilities; never on a live enemy.
- Health: `MaxHealth` and `StartHealthRatio` set after `DefaultAttributes` in `InitGAS`. The `MaxHealth` override is kept on the enemy and re-applied after every `InitializeDefaultAttributes` (reset at zero, `ResetForNewAttempt`), always at full; `StartHealthRatio` is the spawn only.
- Callers of `SpawnEnemy`: `AGeoArena::ResetBoss` (`BossSetup`), `GeoSpawnEnemiesAction` (`Encounter/Action/CLAUDE.md`), `AGeoArena::SpawnAdd` (`Actor/Arena/CLAUDE.md`, owner and `Arena` = the arena; registers the add after; `Arena` goes in before `FinishSpawning` because the StateTree starts on possession and `STTask_SelectNextFiringPoint` reads it), so `AGeoSummonedDummy`'s ally too.

## HiddenCasterCharacter
- Purpose: hazard patterns and waves are abilities and need an ASC; this lends one to a room with no boss. The encounter fires its abilities with `GeoActivateAbilityAction` (`Encounter/Action/CLAUDE.md`).
- Nothing on screen: no mesh, no health bar (subobject not created), no mesh asset (native class, no Blueprint), the actor itself never hidden (a hidden actor also hides the pattern VFX attached to it: the wave would not show). Nothing to hit: `SetInvulnerable(true)` on BeginPlay (no damage, no collision, no gravity), so it never dies. `CanBeAggroed()` false: its controller starts no aggro watch, since aggro would start a fight and make it touchable again.
- No StateTree by default: it acts when an encounter asks. The arena's `BossSetup.StateTree` gives it one to cast on its own loop (the Training Room's zone row).
- Hosted by an `AGeoDummyArena` per room (`BossClass` = `AHiddenCasterCharacter` itself, `ArenaTag` = the room's tag): `Arena` is set, so target-point patterns fire from that room's points. One caster per room that casts; patterns centred on the caster fire from where its arena spawns it.
- No Blueprint: its abilities (and tree, if any) are the arena's `BossSetup` (`Actor/Arena/CLAUDE.md`); enemy team, so its hazards hit players. Only for real hazard patterns (waves, safe spots); a plain zone in a scripted room is `GeoSpawnZoneAction`, no caster.
- Counts as a live enemy: a `GeoActorDiedTrigger` watching `AEnemyCharacter` sees it alive forever; watch the dummy's own class.

## GeoSummonedDummy
- Server BeginPlay: spawns `UGameDataSettings::DefaultZoneClass` at its feet with `ZoneEffects` (damage per second) and `ZoneParams` (start `Size`, growth `GrowToSize` / `GrowSeconds`, `Actor/Deployable/CLAUDE.md`), owned by the dummy (enemy team, so it hurts players).
- The zone dies with the dummy: spawned with the dummy as instigator, it sits in the dummy's `UGeoDeployableManagerComponent`, which `StopAllSpawnedElements` empties on death and on `EndPlay` (a wipe or the boss's death destroys the dummy).
- Death (server): `Arena->SpawnAdd(AllyClass, its transform, AllySetup)`: the ally is an add of the same arena, so the next wipe removes it too. Destroyed without dying: no ally. Tutorial: `AllyClass` = `BP_Dummy`, `AllySetup` = player team, shooting StateTree + projectile ability, `Tag` the encounter watches.
- One Blueprint (`BP_TutorialSummonedDummy`, the dummy's look) holds its zone and ally params; values (`ZoneEffects` amounts, the zone's growth) read `CT_Tutorial` rows through `FScalableFloat`. Its health, tree and salve are the summon ability's `AddSetup` (`Abilities/Boss/CLAUDE.md`).

## Enemy Blueprint defaults
- A default every enemy shares (base `StateTree`, …) is set once on the CDO of `BP_BaseEnemyBlueprintToInheritFrom`; boss Blueprints hold only their own values (`BehaviourStateTree`, …).
- Moving a default up: write the base, recompile it, then recompile and re-save the children so identical overrides drop. A parent default also reaches dummies and tutorial enemies: check the children first.
