# GeoTrinity Implementation Plan

**Last Updated:** 2026-02-04
**Status:** Planning Complete - Ready for Implementation
**Branch:** highfirerateability

---

## How to Resume

Tell Claude: **"Read .claude/memory/IMPLEMENTATION_PLAN.md and continue implementation"**

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

**Passive - Frontal Armor:**
- 25% damage reduction from front
- Reduced rotation speed

**Special - Protective Walls:**
- Fixed number of deployable walls (more = thinner/wider player)
- Walls have health, block enemy projectiles, pass ally projectiles
- Wall orientation = character facing at spawn

**Range Gauge:** Hold to charge, determines deployment distance

**Alt Special - Amplifier Mode:**
- Walls no longer block, instead amplify ally projectiles (stacks)
- Walls still take damage

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
| `UGeoAutomaticFireAbility` | `AbilitySystem/Abilities/Damaging/` | ✅ Hold-to-fire, deterministic sync |
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
| Player Class Enum | `Public/Characters/PlayerClassTypes.h` | [ ] |
| Extended Attributes | `Public/AbilitySystem/AttributeSet/CharacterAttributeSet.h` | [ ] |
| PlayerClass on Character | `Public/Characters/PlayableCharacter.h` | [ ] |
| PlayerClass Replication | `Public/GeoPlayerState.h` | [ ] |
| Range Gauge Component | `Public/AbilitySystem/Component/GeoRangeGaugeComponent.h/.cpp` | [ ] |
| Deployable Base | `Public/Actor/Deployable/GeoDeployableBase.h/.cpp` | [ ] |
| Deployable Manager | `Public/Characters/Component/GeoDeployableManagerComponent.h/.cpp` | [ ] |
| Buff Pickup | `Public/Actor/Pickup/GeoBuffPickup.h/.cpp` | [ ] |

### Phase 2: Common Abilities

| Task | File | Status |
|------|------|--------|
| Basic Attack | `Public/AbilitySystem/Abilities/Common/GeoBasicAttackAbility.h/.cpp` | [ ] |
| Dash | `Public/AbilitySystem/Abilities/Common/GeoDashAbility.h/.cpp` | [ ] |

### Phase 3: Triangle (DPS)

| Task | File | Status |
|------|------|--------|
| Ammo Auto-Attack | `Abilities/Triangle/GeoTriangleAutoAttackAbility.h/.cpp` | [ ] |
| Reload Ability | `Abilities/Triangle/GeoTriangleReloadAbility.h/.cpp` | [ ] |
| Turret Completion | `Actor/Turret/GeoTurretBase.h/.cpp` | [ ] |
| Deploy Turret | `Abilities/Triangle/GeoDeployTurretAbility.h/.cpp` | [ ] |
| Explosive Recall | `Abilities/Triangle/GeoExplosiveRecallAbility.h/.cpp` | [ ] |

### Phase 4: Circle (Healer)

| Task | File | Status |
|------|------|--------|
| Healing Aura | `Abilities/Circle/GeoHealingAuraAbility.h/.cpp` | [ ] |
| Healing Zone | `Actor/Deployable/GeoHealingZone.h/.cpp` | [ ] |
| Deploy Zone | `Abilities/Circle/GeoDeployHealingZoneAbility.h/.cpp` | [ ] |
| Moira Beam | `Abilities/Circle/GeoMoiraBeamAbility.h/.cpp` | [ ] |

### Phase 5: Square (Tank)

| Task | File | Status |
|------|------|--------|
| Frontal Armor | `Abilities/Square/GeoFrontalArmorAbility.h/.cpp` | [ ] |
| Frontal Damage Calc | `ExecCalc/ExecCalc_FrontalDamage.h/.cpp` | [ ] |
| Protective Wall | `Actor/Deployable/GeoProtectiveWall.h/.cpp` | [ ] |
| Deploy Wall | `Abilities/Square/GeoDeployWallAbility.h/.cpp` | [ ] |
| Amplifier Mode | `Abilities/Square/GeoAmplifierModeAbility.h/.cpp` | [ ] |

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

---

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
