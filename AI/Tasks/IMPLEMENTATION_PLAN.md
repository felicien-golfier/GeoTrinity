# GeoTrinity Implementation Plan

**Last Updated:** 2026-04-22
**Status:** Phase 6 Next — Phase 5 (Square) 100% complete (C++ + Blueprint + Data)
**Branch:** Plan-for-design

---

## How to Resume

Tell Claude: **"Read IMPLEMENTATION_PLAN.md and continue implementation"**

---

## Design Requirements

### Common Abilities (All Classes)
- **Basic Attack:** Fast linear projectile with max distance, short cooldown
- **Dash:** Character acceleration WITHOUT invincibility

### Triangle Player (DPS)

**Passive - Ammo & Random Buffs:**
- Limited ammo on auto-attack
- On reload: spawn random buff pickup (DamageBoost, Tankiness, HealBoost, Speed, Heal, Shield)
- Can reload anytime when ammo < max
- Buff power/size scales with missing ammo count

**Special - Turrets:**
- Fixed number of deployable turrets
- More turrets deployed = player becomes smaller
- Turrets attack nearest enemy OR player's current target
- Turrets have health/duration gauge (decreases over time + from damage)
- At gauge = 0: blinks 2 sec, if recalled during blink = 2x damage

**Range Gauge:** Hold to charge, determines deployment distance

**Alt Special - Explosive Recall:** Recalls all turrets with heavy damage along path

### Circle Player (Healer)

**Passive - Healing Aura:**
- HoT to allies in physical contact
- Self-heals % of emitted healing

**Special - Healing Zones:**
- Fixed number of deployable zones (more = smaller player)
- Static HoT zones, gauge depletes only when actively healing
- Zones stack

**Range Gauge:** Hold to charge, determines deployment distance

**Alt Special - Moira Beam:**
- Channeled beam: damages enemies, heals allies/pillars
- Increased movement speed during channel
- Beam width = player diameter
- Absorbing healing zones intensifies beam + extends duration

### Square Player (Tank)

**Passive - Shield Burst Gauge:**
- Auto-attack damage fills a gauge (displayed on character in player color)
- At 100%: launches a moving deployable (like reload buff) that gives shield to allies
- Deployable starts fast and decelerates; interactable from first frame
- On enemy contact: reflects (same angle off enemy tangent) and multiplies shield value by a configurable factor

**Special - Mine:**
- Costs 50% of current HP; cannot activate at minimum HP
- Deploys a small mine via charge gauge (determines distance)
- On any overlap (ally or enemy): explodes over large radius
- Damage to enemies and shield to allies both scale with life spent

**Alt Special - Detonate All Mines:**
- Instantly detonates all deployed mines
- Applies a configurable multiplier (×2 default) to their damage/shield value

### Space Invader Boss

**Movement:** Random direction, stops to cast

**Ability 1 - Targeted Salvo:** Fires at each player's position every 0.X seconds

**Ability 2 - Spiral:** Projectile spiral around boss

**Ability 3 - Delayed Fatal Zones:** Zone under each player, lethal damage after X sec, spawns Pillar

**Pillars:** Have health, can be damaged/healed by players, damaged by boss

**Special - Devastating Wave:**
- Boss teleports to center, lethal expanding wave
- Pillars block wave (safe zone behind), then explode (semi-lethal AoE)

---

## Codebase Analysis

### Already Implemented

| System | Location | Status |
|--------|----------|--------|
| `UGeoGameplayAbility` | `AbilitySystem/Abilities/` | ✅ Base with StoredPayload, ScheduleFireTrigger, SendFireDataToServer, OnFireTargetDataReceived |
| `UGeoProjectileAbility` | `AbilitySystem/Abilities/Damaging/` | ✅ Single-shot: client fires in Fire(), server fires in OnFireTargetDataReceived() |
| `UGeoAutomaticFireAbility` | `AbilitySystem/Abilities/Damaging/` | ✅ Abstract hold-to-fire loop; subclass overrides ExecuteShot() |
| `UGeoAutomaticProjectileAbility` | `AbilitySystem/Abilities/Damaging/` | ✅ Concrete auto-fire spawning projectiles |
| `FGeoAbilityTargetData` | `AbilitySystem/Data/` | ✅ Per-shot RPC payload (Origin, Yaw, ServerSpawnTime, Seed) sent client→server |
| `AGeoProjectile` | `Actor/Projectile/` | ✅ IsNetRelevantFor excludes owner; AdvanceProjectile for time-sync |
| `AGeoTurretBase` | `Actor/Turret/` | ⚠️ Skeleton (needs targeting, gauge, blink) |
| `USpiralPattern` | `AbilitySystem/Abilities/Pattern/` | ✅ Bullet spiral |
| `AGeoInteractableActor` | `Actor/` | ✅ Base for ASC/health/team actors |
| `UGeoActorPoolingSubsystem` | `System/` | ✅ Actor pooling |
| GAS Infrastructure | `AbilitySystem/` | ✅ ASC, Attributes, Effects, Projectiles |
| StateTree AI | `AI/StateTree/` | ✅ Base tasks exist |

### Key Patterns

**Deployable:** Extend `AGeoInteractableActor` + `IGeoPoolableInterface` with `Init()`/`End()`

**Pooled Spawning:**
```cpp
auto* Actor = UGeoActorPoolingSubsystem::Get(GetWorld())
    ->RequestActor(Class, Transform, Owner, Instigator, false);
Actor->Init();
```

**Network Sync:** Use `FAbilityPayload` (Origin, Yaw, ServerSpawnTime, Seed)

**StateTree Async:** `bShouldCallTick = false` + `MakeWeakExecutionContext()` + `FinishTask()`

---

## Implementation Phases

### Phase 1: Foundation

| Task | File | Status |
|------|------|--------|
| Player Class Enum | `Public/Characters/PlayerClassTypes.h` | [x] |
| Extended Attributes | `Public/AbilitySystem/AttributeSet/CharacterAttributeSet.h` | [x] |
| PlayerClass on Character | `Public/Characters/PlayableCharacter.h` | [x] |
| PlayerClass Replication | `Public/GeoPlayerState.h` | [x] |
| Range Gauge Component | Deleted — charge logic will live in deploy abilities | N/A |
| Deployable Base | `Public/Actor/Deployable/GeoDeployableBase.h/.cpp` | [x] |
| Deployable Manager | `Public/Characters/Component/GeoDeployableManagerComponent.h/.cpp` | [x] |
| Buff Pickup | Is now a Deployable ! `Public/Actor/Pickup/GeoBuffPickup.h/.cpp` | [x] |
| Gameplay Tags | `Public/AbilitySystem/Lib/GeoGameplayTags.h/.cpp` | [x] |

### Phase 2: Common Abilities

| Task | File | Status |
|------|------|--------|
| Basic Attack | N/A - Use existing BP abilities + `EPlayerClass` filter in `FGameplayAbilityInfo` | [x] Already exists |
| Dash | `Public/AbilitySystem/Abilities/Common/GeoDashAbility.h/.cpp` | [x] |

**Note**: Class-specific ability selection is done via `EPlayerClass PlayerClass` field in `FGameplayAbilityInfo`. Multiple abilities can share the same `AbilityTag` but differ by `PlayerClass`. Do NOT create separate C++ classes for each player class's basic/special abilities.

### Phase 3: Triangle (DPS)

**Note on auto-attack**: `UGeoAutomaticProjectileAbility` already exists and handles the shoot loop + netcode. Triangle auto-attack should extend `UGeoAutomaticFireAbility` with a custom `ExecuteShot()` that integrates ammo consumption. Do NOT duplicate the netcode — just override `ExecuteShot()` (and use ability cost for ammo tracking).

**Note on deploy**: `UGeoDeployAbility` (Common, `Abilities/Common/GeoDeployAbility.h`) handles hold-to-charge + release to deploy for all classes. It fires a `TurretSpawnerProjectile` which spawns the turret on impact. No class-specific deploy ability needed — use `EPlayerClass` filter on the data asset.

| Task | File | Status |
|------|------|--------|
| Ammo Auto-Attack | No C++ needed — `UGeoAutomaticFireAbility` already calls `CommitAbilityCost()` per shot and ends when ammo runs out. Just configure CostGE on the `UGeoAutomaticProjectileAbility` BP. | [BP] |
| Reload Ability | `Abilities/Triangle/GeoTriangleReloadAbility.h/.cpp` | [x] Complete |
| Turret C++ | `Actor/Turret/GeoTurretBase.h/.cpp` | [x] Complete |
| Turret Spawner Projectile | `Actor/Projectile/TurretSpawnerProjectile.h/.cpp` | [x] Complete |
| Deploy Ability (Common) | `Abilities/Common/GeoDeployAbility.h/.cpp` | [x] Complete |
| Explosive Recall | `Abilities/Triangle/GeoExplosiveRecallAbility.h/.cpp` | [x] Complete |
| **[BP] Create BP subclasses** of all new ability classes | Editor | [x] |
| **[BP] Configure UAbilityInfo** DA with EPlayerClass::Triangle filter | DA_AbilityInfo | [x] |
| **[BP] Set up UEffectDataAssets** (ammo cost GE, ammo restore GE, buff effects, recall normal/blink-bonus) | Editor | [x] |
| **[BP] GameplayCue BP** for recall beam VFX (RecallGameplayCueTag) | Editor | [x] |
| **[BP] Turret BP subclass** with TurretProjectileClass + FireInterval | Editor | [x] |

### Phase 4: Circle (Healer)

| Task | File | Status |
|------|------|--------|
| Healing Aura | `Abilities/Circle/GeoHealingAuraAbility.h/.cpp` | [x] |
| Healing Zone | `Actor/Deployable/GeoHealingZone.h/.cpp` | [x] |
| Healing Zone Spawner Projectile | `Actor/Projectile/DeployableSpawner/HealingZoneSpawnerProjectile.h/.cpp` | [x] |
| Deploy Zone | No C++ needed — reuse `UGeoDeployAbility` directly (same as Triangle) | [BP] |
| Moira Beam | `Abilities/Circle/GeoMoiraBeamAbility.h/.cpp` | [x] |
| Charge Beam | `Abilities/Circle/GeoChargeBeamAbility.h/.cpp` + `UGeoChargeAbility` base | [x] |
| **[BP] Ability BPs + DA_AbilityInfo** entries with EPlayerClass::Circle filter | Editor | [x] |
| **[BP] GE assets** — HealGE (SetByCaller Gameplay_Heal), SpeedBuff GE (modify MovementSpeedMultiplier) | Editor | [x] |
| **[BP] HealingZone BP** subclass — set DeployableActorClass, EffectDataArray, HealRadius, HealTickInterval | Editor | [x] |

### Phase 5: Square (Tank) ✅ COMPLETE

| Task | File | Status |
|------|------|--------|
| Shield Burst Passive | `Abilities/Square/GeoShieldBurstPassiveAbility.h/.cpp` | [x] |
| Shield Burst Projectile | `Actor/Projectile/GeoShieldBurstProjectile.h/.cpp` | [x] |
| Mine | `Actor/Deployable/GeoMine.h/.cpp` | [x] |
| Mine Ability | `Abilities/Square/GeoMineAbility.h/.cpp` | [x] |
| Detonate All Mines | `Abilities/Square/GeoDetonateAllMinesAbility.h/.cpp` | [x] |

### Phase 6: Boss

| Task | File | Status |
|------|------|--------|
| Boss Character | `Characters/Boss/GeoSpaceInvaderBoss.h/.cpp` | [ ] |
| Boss Movement Task | `AI/StateTree/Boss/STTask_BossRandomMovement.h/.cpp` | [ ] |
| Boss Ability Select | `AI/StateTree/Boss/STTask_BossSelectAbility.h/.cpp` | [ ] |
| Boss Teleport Task | `AI/StateTree/Boss/STTask_BossTeleportToCenter.h/.cpp` | [ ] |
| Targeted Salvo | `Abilities/Boss/GeoTargetedSalvoAbility.h/.cpp` | [ ] |
| Delayed Fatal Zone | `Abilities/Boss/GeoDelayedFatalZoneAbility.h/.cpp` | [ ] |
| Fatal Zone Actor | `Actor/Boss/GeoFatalZone.h/.cpp` | [ ] |
| Pillar Actor | `Actor/Boss/GeoPillar.h/.cpp` | [ ] |
| Devastating Wave | `Abilities/Boss/GeoDevastatingWaveAbility.h/.cpp` | [ ] |
| Wave Actor | `Actor/Boss/GeoDevastatingWave.h/.cpp` | [ ] |

### Phase 7: Multiplayer Connection Pipeline

Full design in `project_multiplayer_pipeline.md` and `C:\Users\Felou\.claude\plans\i-want-you-to-sequential-neumann.md`.

**Model:** EOS internet, listen server, host/join-by-code, one game map (staging zone + boss room), late join always spawns in staging zone.

| Task | File | Status |
|------|------|--------|
| Add OnlineSubsystem modules | `GeoTrinity.Build.cs` | [ ] |
| EOS ini config | `Config/DefaultEngine.ini` | [ ] |
| UGeoSessionSubsystem | `Public/System/GeoSessionSubsystem.h/.cpp` | [ ] |
| AGeoMainMenuGameMode | `Public/GeoMainMenuGameMode.h/.cpp` | [ ] |
| AGeoGameMode: PostLogin, Logout, ChoosePlayerStart | `Public/GeoGameMode.h`, `Private/GeoGameMode.cpp` | [ ] |
| **[BP] MainMenuMap** | Editor | [ ] |
| **[BP] BP_MainMenuGameMode** (subclass AGeoMainMenuGameMode) | Editor | [ ] |
| **[BP] BP_MainMenuWidget** (Host + Join-by-code UI) | Editor | [ ] |
| **[BP] Staging PlayerStarts** in GameMap (tag = "Staging") | Editor | [ ] |
| **[EOS] Register product & enter credentials** | dev.epicgames.com + Project Settings | [ ] |
| **[BP] Set Game Default Map** = MainMenuMap | Project Settings | [ ] |

---

## Post-Plan Notes

- **Live class switching**: EPlayerClass is set on the character but nothing reacts to it yet. Goal: switching the enum live should update abilities, attributes, size scaling, deployable limits, etc. — do after all phases are done.

## Gameplay Tags to Add

```cpp
// Player classes
PlayerClass.Triangle / .Circle / .Square

// Input tags
InputTag.Reload / .Deploy / .AltSpecial

// Buff status
Status.Buff.DamageBoost / .Tankiness / .HealBoost / .Speed / .Shield

// Boss abilities
Ability.Boss.TargetedSalvo / .Spiral / .FatalZones / .DevastatingWave
```

---

## Session Notes

### 2026-02-04
- Initial planning session
- Analyzed existing codebase
- Created implementation plan
- Ready to begin Phase 1

### 2026-02-06
- Phase 2: Common Abilities implemented
- Basic Attack: Uses existing BP abilities + EPlayerClass filter in FGameplayAbilityInfo (no new C++ class needed)
- Created GeoDashAbility (velocity impulse via LaunchCharacter, no invincibility)
- Updated CLAUDE.md with class-specific ability selection pattern
- Build verified successful
- Ready for Phase 3: Triangle (DPS)

### 2026-03-09
- Returned from 2-week break, audited Phase 3 progress
- C++ complete: GeoTriangleReloadAbility, GeoExplosiveRecallAbility, GeoTurretBase, TurretSpawnerProjectile, GeoDeployAbility (Common), GeoDeployableBase, GeoDeployableManagerComponent, GeoBuffPickup
- Still missing C++: GeoTriangleAutoAttackAbility (ammo-based auto-fire, extends UGeoAutomaticFireAbility)
- Remaining work is BP/data: ability BPs, DA_AbilityInfo entries, GE assets, GameplayCue BP, turret BP

### 2026-02-23
- Reviewed updated netcode system for player abilities
- System: client fires immediately with predicted data → sends FGeoAbilityTargetData (Origin, Yaw, ServerSpawnTime, Seed) to server via ServerSetReplicatedTargetData → server spawns authoritative projectile in OnFireTargetDataReceived
- Server projectile excluded from owning client via IsNetRelevantFor (client keeps local predicted one)
- UGeoAutomaticProjectileAbility confirmed as existing concrete class for auto-fire projectile abilities
- Triangle auto-attack should extend UGeoAutomaticFireAbility and override ExecuteShot() — do not duplicate netcode
- Updated CLAUDE.md Networking Model and Ability Classes sections

### 2026-03-30
- Montage sync fix completed (FIX_MONTAGE_SYNC_PLAN resolved)
- Phase 3 Triangle: 100% complete — all C++ and all Blueprint/Data work done
- Class names differ slightly from plan: GeoTurretBase→AGeoTurret, GeoExplosiveRecallAbility→GeoRecallTurretAbility, GeoTriangleReloadAbility→GeoReloadAbility
- Ready for Phase 4: Circle (Healer)
