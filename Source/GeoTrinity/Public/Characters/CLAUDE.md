# Characters

```
AGeoCharacter            ASC interface + team agent
├── APlayableCharacter   ASC on PlayerState, Enhanced Input, runtime class switch
└── AEnemyCharacter      own ASC (minimal replication), StateTree AI
    ├── AHiddenCasterCharacter   invisible, untouchable ability caster for scripted rooms
    ├── AGeoSummonedDummy        summoned add: growing damage zone at its feet, turns ally on death
    └── AGeoTrainingCat          the Training Room's Cat: puns, warns when hit, chases and kills when hit again, goes home
```

| File | Role |
|---|---|
| `GeoCharacter.h` | Abstract base: ASC, input, movement, health bar, game feel, turn-rate clamp |
| `PlayableCharacter.h` | Human player: input forwarding, `ChangeClass()`, gauges, downed/revive |
| `EnemyCharacter.h` | AI enemy: own ASC, StateTree, boss death / reset |
| `GeoEnemySetup.h` | `FGeoEnemySetup`: per-spawn variant of an enemy class (team, dies or resets, StateTree, extra abilities, health, actor tag) |
| `HiddenCasterCharacter.h` | Enemy with nothing on screen and nothing to hit: casts abilities for an encounter in a room with no boss |
| `GeoSummonedDummy.h` | Add summoned by `GeoSummonAddAbility` (tutorial miniboss): its zone grows until it dies, then an ally takes its place |
| `GeoTrainingCat.h` | The Training Room's Cat: enemy-team character, hit detection, spoken line replication, home spot |
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

## Friendly look
- Purpose: a dummy on the player team (hurt ally, summoned dummy's ally, shooting ally) looks different from an enemy one; the look follows the team, never a Blueprint per variant (`BP_Dummy` stays one Blueprint).
- `AEnemyCharacter::FriendlyBodyMesh` / `FriendlyBodyMaterial` (`USkeletalMesh` / material set on `BP_Dummy`: `SKM_SoftStar` + `MI_SoftStar_Body`; the mesh's own Core slot keeps the pale disc; `AI/ArtDirection.md` "Dummies"): applied to `GetMesh()` when `bFriendlyLook`, the enemy look otherwise. Empty = no change.
- `TeamId` does not replicate, so the look needs its own replicated flag `bFriendlyLook` (`OnRep_FriendlyLook`): set on the server by `ApplySetup`, the one place an enemy's team is set; the OnRep applies the look on clients. Server and clients run the same `ApplyFriendlyLook()`.
- `SetSkeletalMesh` + `SetMaterial(0)`, never a per-instance asset; death / reset-at-zero keep the look (the team does not change).

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

## GeoTrainingCat
- Purpose: the Training Room's Cat (`AI/Tutorial/Cat.md`, "Training Room Cat"). Not `AGeoCat` (the solo tutorial's non-replicated puppet): a real enemy-team character, so hits, damage, replication and the arena reset already work. Same look (`SKM_Cat` + `ABP_Cat`), one Blueprint `BP_TrainingCat` (look and tunables only), everything else `FGeoEnemySetup` on the room arena's `StartAdds`.
- Setup: `AggroRadius` -1 (nothing wakes it by proximity), `bResetToFullLife` (it cannot be killed), `StateTree` `ST_Training_Cat`, `AbilityTags` = warning + strike. Its behaviour is the tree; the class holds only what a tree cannot: hit detection, speech replication, home, anim speed.
- Home = the location it spawned at (`BeginPlay`, server): the room arena's `TargetPoint.AddSpawn` for it, so "its place" is a point the user moves in the editor.
- Hit: server, `OnGameplayEffectAppliedDelegateToSelf` on its ASC; an effect whose source ASC is not its own and whose source team is hostile to it raises the StateTree event `AI.Cat.Hit` (a heal from a friend is not a hit). The controller's aggro latch (`bAggroed`) fires once, so the tree never listens to aggro.
- Speech: `Speak(Categories)` (server): picks `GeoCatLines::PickLine(Categories, true)`, stores `Speech` (`FText`) + `SpeechSerial` (replicated; the serial makes the same text twice a new event). `OnRep_Speech` and the server call the same `ShowSpeech`: first local player's HUD `ShowCatLine` (`GeoTrinityUI/Public/HUD/Tutorial/CLAUDE.md`), only when that player is within `HearRange` (2D). Nobody within `HearRange`: nothing picked, nothing drawn (no wasted bag line). `OnSpoken` (native multicast, every machine) = what tests read, as `UGeoEncounterSubsystem::OnCatLine`.
- Anim speed: writes `Speed` (real speed, cm/s) on `ABP_Cat` each tick with the same by-name helper as `AGeoCat` (`Actor/GeoCatAnim.h`, lifted out of `GeoCat.cpp`, one copy); `WalkSpeed` calm, `ChaseSpeed` while chasing (the AnimBP picks Prowl / Fast by speed, `CatAnimation.md`).
- Health bar hidden; the capsule is the hitbox (no extra collision).

### Behaviour (`ST_Training_Cat`, built by `AI/Python/Tutorial/Level/training_room.py`)
```
Calm     Delay(PunInterval, random) -> Pun
Pun      Speak(Pun)                 -> Calm
  Calm / Pun on AI.Cat.Hit          -> Warned
Warned   Speak(Retort) + Fire(Warning ability), ALL -> Dare      (a hit here is ignored: a held trigger is not a dare)
Dare     Delay(DareWindow)          -> Calm;  on AI.Cat.Hit -> Chase
Chase    ChaseTarget + StrikeInRange (+ Delay(ChaseTimeout)), ANY -> Return
Return   MoveToHome                 -> Calm
```
- Warning: non-lethal burst around the cat (`GA_TrainingCat_Warning`, the Training Room's burst pattern, light damage, ring larger than the strike): "back off". Strike: `GA_TrainingCat_Strike`, same pattern kind, claw-sized radius, the lethal effect of the Training Room's Lethal zone, near-instant telegraph.
- Chase target = the controller's `GetCurrentTarget()` (nearest live player; `AI/CLAUDE.md`); straight line, no navmesh (the room has none). A kill, no live player, or `ChaseTimeout` ends the chase.
- Return: hits are ignored until it is home (no ping-pong); home -> Calm, the next hit starts again at the warning. The arena reset (`ResetAdds`) respawns it at its point like any add.
