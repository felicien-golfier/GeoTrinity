# GeoTrinity Implementation Project

**Last Updated:** 2026-02-04
**Current Phase:** Planning Complete - Ready for Implementation
**Branch:** highfirerateability

---

## Quick Start for New Sessions

Tell Claude: "Read `C:\Users\Felou\.claude\projects\C--GeoTrinity\memory\GEOTRINITY_PROJECT.md` to continue the GeoTrinity project"

---

## Design Document (Original Requirements)

### Triangle Player (DPS)

**Passive - Ammo & Random Buffs:**
- Limited ammo on auto-attack
- On reload: spawn random buff pickup nearby (dmg boost, tankiness, heal boost, speed, heal, shield)
- Can reload anytime when ammo missing
- Buff power/size scales with missing ammo count

**Special - Turrets:**
- Fixed number of deployable turrets
- More turrets deployed = player becomes smaller
- Turrets attack nearest enemy or player's target
- Turrets have health/duration gauge (decreases over time + damage)
- At 0 health: blinks 2 sec, if recalled during blink = 2x damage

**Range Gauge:** Charge determines deployment distance

**Alt Special - Explosive Recall:**
- Recalls all turrets to player
- Heavy damage to enemies crossed

### Circle Player (Healer)

**Passive - Healing Aura:**
- HoT to allies in physical contact
- Self-heals % of emitted healing

**Special - Healing Zones:**
- Fixed number of deployable zones
- More zones = player becomes smaller
- Static HoT zones on ground
- Each zone has healing gauge (consumed only when healing)
- Zones can stack

**Range Gauge:** Charge determines deployment distance

**Alt Special - Moira Beam:**
- Channeled beam: heals allies, damages enemies
- Heals neutral elements (pillars)
- Increases movement speed during channel
- Beam width = player diameter
- Absorbing healing zones intensifies beam + extends duration

### Square Player (Tank)

**Passive - Frontal Armor:**
- 25% damage reduction from front
- Reduced rotation speed

**Special - Protective Walls:**
- Fixed number of deployable walls
- More walls = player thinner and wider
- Walls have health
- Block enemy projectiles, allow ally projectiles through

**Range Gauge:** Charge determines deployment distance
**Wall orientation:** Character facing at spawn

**Alt Special - Amplifier Mode:**
- Walls no longer block damage
- Walls amplify ally projectiles passing through
- Bonus stacks with multiple walls
- Walls still take damage

### Common Abilities (All Classes)
- **Basic Attack:** Fast linear projectile, max distance, short cooldown
- **Dash:** Acceleration without invincibility

### Space Invader Boss

**Movement:** Continuous random direction, stops to cast

**Ability 1 - Targeted Salvo:**
- Fires at each player's current position every 0.X seconds

**Ability 2 - Spiral:**
- Projectile spiral around boss

**Ability 3 - Delayed Fatal Zones:**
- Zone under each player
- After X seconds: lethal damage + spawns Pillar

**Pillars:**
- Have health bar
- Can be damaged/healed by players, damaged by boss

**Special - Devastating Wave:**
- Boss teleports to center
- Lethal expanding wave in all directions
- Pillars block wave (create safe zone behind)
- Pillars explode after absorbing (semi-lethal AoE)

---

## Codebase Analysis

### Already Implemented

| System | Location | Status |
|--------|----------|--------|
| `UGeoAutomaticFireAbility` | `AbilitySystem/Abilities/Damaging/` | ✅ Complete - hold-to-fire with deterministic sync |
| `AGeoTurretBase` | `Actor/Turret/` | ✅ Skeleton - needs targeting/firing logic |
| `USpiralPattern` | `AbilitySystem/Abilities/Pattern/` | ✅ Complete - bullet spiral |
| `AGeoInteractableActor` | `Actor/` | ✅ Complete - base for actors with ASC/health/team |
| `UGeoActorPoolingSubsystem` | `System/` | ✅ Complete - actor pooling |
| GAS Infrastructure | `AbilitySystem/` | ✅ Complete - ASC, Attributes, Effects, Projectiles |
| StateTree AI | `AI/StateTree/` | ✅ Base tasks exist |
| `APlayableCharacter` | `Characters/` | ✅ Single class for all players |
| `AEnemyCharacter` | `Characters/` | ✅ Boss base class |

### Key Base Classes

- `UGeoGameplayAbility` - Base ability class
- `UGeoProjectileAbility` - Spawns projectiles
- `UGeoAutomaticFireAbility` - Hold-to-fire pattern
- `UPatternAbility` - Bullet patterns via multicast
- `AGeoInteractableActor` - Actors with ASC/health/team
- `IGeoPoolableInterface` - Pooling callbacks (Init/End)
- `FAbilityPayload` - Network sync (Origin, Yaw, ServerSpawnTime, Seed)

### Key File Paths

```
Source/GeoTrinity/
├── Public/Private AbilitySystem/
│   ├── Abilities/
│   │   ├── GeoGameplayAbility.h         # Base ability
│   │   ├── Damaging/
│   │   │   ├── GeoProjectileAbility.h   # Projectile spawning
│   │   │   └── GeoAutomaticFireAbility.h # Hold-to-fire
│   │   └── Pattern/
│   │       ├── Pattern.h                # UTickablePattern base
│   │       └── SpiralPattern.h          # Spiral projectiles
│   ├── AttributeSet/
│   │   ├── GeoAttributeSetBase.h        # Health, MaxHealth, IncomingDamage
│   │   └── CharacterAttributeSet.h      # Extend this for new attributes
│   └── Data/
│       ├── EffectData.h                 # FDamageEffectData, FStatusEffectData
│       └── AbilityPayload.h             # Network sync payload
├── Actor/
│   ├── GeoInteractableActor.h           # Base for deployables
│   ├── Projectile/GeoProjectile.h       # Projectile actor
│   └── Turret/GeoTurretBase.h           # Turret skeleton
├── Characters/
│   ├── GeoCharacter.h                   # Base character
│   ├── PlayableCharacter.h              # Player character
│   └── EnemyCharacter.h                 # Enemy/boss base
├── AI/StateTree/
│   ├── STTask_FireProjectileAbility.h   # Fire ability by tag
│   └── STTask_SelectNextFiringPoint.h   # Round-robin positions
└── System/
    ├── GeoActorPoolingSubsystem.h       # Actor pooling
    └── GeoPoolableInterface.h           # Init/End callbacks
```

---

## Implementation Plan

### Phase 1: Foundation Systems

**New Files:**
- `Public/Characters/PlayerClassTypes.h` - `EPlayerClass` enum (Triangle, Circle, Square)
- `Public/AbilitySystem/Component/GeoRangeGaugeComponent.h/.cpp` - Charge-based deployment distance
- `Public/Characters/Component/GeoDeployableManagerComponent.h/.cpp` - Track deployed actors per player
- `Public/Actor/Deployable/GeoDeployableBase.h/.cpp` - Base for turrets/walls/zones
- `Public/Actor/Pickup/GeoBuffPickup.h/.cpp` - Random buff pickups

**Modify:**
- `PlayableCharacter.h` - Add `EPlayerClass PlayerClass`
- `GeoPlayerState.h` - Add replicated player class
- `CharacterAttributeSet.h` - Add: Ammo, MaxAmmo, Shield, HealingPower, DamageReduction, MovementSpeedMultiplier, RotationSpeedMultiplier

### Phase 2: Common Abilities

**New Files:**
- `Public/AbilitySystem/Abilities/Common/GeoBasicAttackAbility.h/.cpp`
- `Public/AbilitySystem/Abilities/Common/GeoDashAbility.h/.cpp`

### Phase 3: Triangle (DPS)

**New Files:**
- `Abilities/Triangle/GeoTriangleAutoAttackAbility.h/.cpp` - Extends GeoAutomaticFireAbility, uses Ammo
- `Abilities/Triangle/GeoTriangleReloadAbility.h/.cpp` - Reload + buff spawn
- `Abilities/Triangle/GeoDeployTurretAbility.h/.cpp`
- `Abilities/Triangle/GeoExplosiveRecallAbility.h/.cpp`

**Modify:**
- `Actor/Turret/GeoTurretBase.h/.cpp` - Add targeting, health gauge, blink state

### Phase 4: Circle (Healer)

**New Files:**
- `Abilities/Circle/GeoHealingAuraAbility.h/.cpp`
- `Abilities/Circle/GeoDeployHealingZoneAbility.h/.cpp`
- `Abilities/Circle/GeoMoiraBeamAbility.h/.cpp`
- `Actor/Deployable/GeoHealingZone.h/.cpp`

### Phase 5: Square (Tank)

**New Files:**
- `Abilities/Square/GeoFrontalArmorAbility.h/.cpp`
- `Abilities/Square/GeoDeployWallAbility.h/.cpp`
- `Abilities/Square/GeoAmplifierModeAbility.h/.cpp`
- `Actor/Deployable/GeoProtectiveWall.h/.cpp`
- `ExecCalc/ExecCalc_FrontalDamage.h/.cpp`

### Phase 6: Space Invader Boss

**New Files:**
- `Characters/Boss/GeoSpaceInvaderBoss.h/.cpp`
- `AI/StateTree/Boss/STTask_BossRandomMovement.h/.cpp`
- `AI/StateTree/Boss/STTask_BossSelectAbility.h/.cpp`
- `AI/StateTree/Boss/STTask_BossTeleportToCenter.h/.cpp`
- `Abilities/Boss/GeoTargetedSalvoAbility.h/.cpp`
- `Abilities/Boss/GeoDelayedFatalZoneAbility.h/.cpp`
- `Abilities/Boss/GeoDevastatingWaveAbility.h/.cpp`
- `Actor/Boss/GeoFatalZone.h/.cpp`
- `Actor/Boss/GeoPillar.h/.cpp`
- `Actor/Boss/GeoDevastatingWave.h/.cpp`

---

## Progress Tracking

### Phase 1: Foundation
- [ ] PlayerClassTypes.h
- [ ] GeoRangeGaugeComponent
- [ ] GeoDeployableManagerComponent
- [ ] GeoDeployableBase
- [ ] GeoBuffPickup
- [ ] CharacterAttributeSet extensions
- [ ] PlayableCharacter modifications

### Phase 2: Common Abilities
- [ ] GeoBasicAttackAbility
- [ ] GeoDashAbility

### Phase 3: Triangle
- [ ] GeoTriangleAutoAttackAbility
- [ ] GeoTriangleReloadAbility
- [ ] GeoTurretBase completion
- [ ] GeoDeployTurretAbility
- [ ] GeoExplosiveRecallAbility

### Phase 4: Circle
- [ ] GeoHealingAuraAbility
- [ ] GeoHealingZone
- [ ] GeoDeployHealingZoneAbility
- [ ] GeoMoiraBeamAbility

### Phase 5: Square
- [ ] GeoFrontalArmorAbility
- [ ] ExecCalc_FrontalDamage
- [ ] GeoProtectiveWall
- [ ] GeoDeployWallAbility
- [ ] GeoAmplifierModeAbility

### Phase 6: Boss
- [ ] GeoSpaceInvaderBoss
- [ ] Boss StateTree tasks (3)
- [ ] GeoTargetedSalvoAbility
- [ ] GeoDelayedFatalZoneAbility + GeoFatalZone
- [ ] GeoPillar
- [ ] GeoDevastatingWaveAbility + GeoDevastatingWave

---

## Key Patterns to Follow

### Deployable Actor Pattern
```cpp
class AMyDeployable : public AGeoDeployableBase
{
    virtual void Init() override;  // Called when spawned from pool
    virtual void End() override;   // Called when returned to pool
    virtual void OnRecalled() override;  // Called by player recall
};
```

### Ability with Deployable Spawning
```cpp
void UMyDeployAbility::ActivateAbility(...)
{
    float Distance = RangeGaugeComponent->GetCurrentDistance();
    FVector SpawnLocation = GetAvatarLocation() + GetAvatarForward() * Distance;

    auto* Deployable = UGeoActorPoolingSubsystem::Get(GetWorld())
        ->RequestActor(DeployableClass, FTransform(SpawnLocation), Owner, Instigator);

    DeployableManager->RegisterDeployable(Deployable);
}
```

### StateTree Task Pattern (Async)
```cpp
USTRUCT()
struct FSTTask_MyTask : public FStateTreeTaskCommonBase
{
    FSTTask_MyTask() { bShouldCallTick = false; }

    virtual EStateTreeRunStatus EnterState(...) override
    {
        auto WeakContext = Context.MakeWeakExecutionContext();
        // Bind to delegate, call WeakContext.FinishTask() when done
        return EStateTreeRunStatus::Running;
    }
};
```

---

## Notes & Decisions

- All three player classes use same `APlayableCharacter` base, differentiated by `EPlayerClass`
- Deployables use actor pooling for performance
- Network sync via `FAbilityPayload` (deterministic, no per-projectile RPCs)
- Boss uses StateTree for AI behavior phases
