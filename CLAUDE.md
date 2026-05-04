# CLAUDE.md

## Project
GeoTrinity — multiplayer 2D bullet-hell, Unreal Engine 5.7, GAS. Players are geometric shapes (Tank=Square, Heal=Circle, DPS=Triangle). Camera: orthographic, pitch = -90.

## Big RULES
When opening a cpp file, read it entirely.
if you read directly .cpp without going to the .h folder, read the CLAUDE.md in the corresponding public folder
Each source/public sub folder contains a CLAUDE.md that tells you what's inside
When creating a memory, place it in the relevant subfolder's CLAUDE.md.

## Key References
| Context | File |
|---|---|
| Code style, GAS conventions, error handling | `AI/CodingStyle.md` |
| Build commands, dev environment, copyright | `AI/Commands.md` |
| Networking, data structures, effect system | `AI/Architecture.md` |
| VFX / Niagara via MCP | `AI/VFX.md` |
| AbilitySystem code | `Source/GeoTrinity/Public/AbilitySystem/CLAUDE.md` |
| Abilities (all classes) | `Source/GeoTrinity/Public/AbilitySystem/Abilities/CLAUDE.md` |
| Characters & components | `Source/GeoTrinity/Public/Characters/CLAUDE.md` |
| Actors (projectiles, deployables, turret) | `Source/GeoTrinity/Public/Actor/CLAUDE.md` |
| AI & StateTree | `Source/GeoTrinity/Public/AI/CLAUDE.md` |
| HUD & widgets | `Source/GeoTrinity/Public/HUD/CLAUDE.md` |
| Subsystems & pooling | `Source/GeoTrinity/Public/System/CLAUDE.md` |

## Source Structure
```
Source/GeoTrinity/
├── Public/ & Private/
│   ├── AbilitySystem/
│   │   ├── Abilities/
│   │   │   ├── Base/          # GeoGameplayAbility, PatternAbility, AbilityPayload
│   │   │   ├── Damaging/      # GeoProjectileAbility, GeoAutomaticFireAbility, GeoAutomaticProjectileAbility
│   │   │   ├── Pattern/       # Pattern, SpiralPattern
│   │   │   ├── Circle/        # GeoHealingAuraAbility, GeoMoiraBeamAbility, GeoChargeBeamAbility, GeoHealReturnPassiveAbility
│   │   │   ├── Square/        # GeoMineAbility, GeoShieldBurstPassiveAbility, GeoDetonateAllMinesAbility
│   │   │   ├── Triangle/      # GeoReloadAbility, GeoRecallTurretAbility
│   │   │   └── Common/        # GeoDashAbility, GeoDeployAbility
│   │   ├── AttributeSet/      # GeoAttributeSetBase, CharacterAttributeSet
│   │   ├── Components/        # GeoAbilitySystemComponent
│   │   ├── Data/              # AbilityInfo, EffectData, GeoAbilityTargetTypes, StatusInfo
│   │   ├── ExecCalc/          # ExecCalc_Damage, ExecCalc_Heal
│   │   ├── Globals/           # GeoAbilitySystemGlobals
│   │   ├── Lib/               # GeoAbilitySystemLibrary, GeoGameplayTags
│   │   └── Types/             # GeoAscTypes (FGeoGameplayEffectContext)
│   ├── Actor/
│   │   ├── Projectile/        # GeoProjectile, GeoPooledProjectile, GeoShieldBurstProjectile, DeployableSpawnerProjectile
│   │   ├── Deployable/        # GeoDeployableBase, GeoMine, GeoHealingZone, GeoBuffPickup
│   │   ├── Turret/            # GeoTurret
│   │   ├── GeoClassChangeTrigger.h
│   │   └── GeoInteractableActor.h
│   ├── Characters/
│   │   ├── Component/         # GeoCharacterMovementComponent, GeoDeployableManagerComponent, GeoGameFeelComponent, ShieldBurstPassiveComponent
│   │   ├── GeoCharacter.h
│   │   ├── PlayableCharacter.h
│   │   ├── EnemyCharacter.h
│   │   └── PlayerClassTypes.h # EPlayerClass enum
│   ├── AI/
│   │   ├── StateTree/         # STTask_FireProjectileAbility, STTask_SelectNextFiringPoint
│   │   ├── Tasks/             # Legacy BT tasks (BTTask_*)
│   │   └── GeoEnemyAIController.h
│   ├── HUD/
│   │   ├── Component/         # GeoCombattantWidgetComp
│   │   ├── GeoHUD.h
│   │   ├── GeoUserWidget.h
│   │   ├── GenericCombattantWidget.h
│   │   ├── GeoDeployChargeGaugeWidget.h
│   │   └── HudFunctionLibrary.h
│   ├── Input/                 # GeoInputComponent
│   ├── Settings/              # GameDataSettings
│   ├── System/                # GeoActorPoolingSubsystem, GeoPoolableInterface, GeoCombatStatsSubsystem
│   ├── Tool/                  # UGeoGameplayLibrary, GeoAssetManager, Team
│   ├── World/                 # GeoGameCamera, GeoWorldSettings
│   ├── Animation/             # FireAnimNotify
│   └── GameClasses/           # GeoGameMode, GeoGameState, GeoGameInstance, GeoPlayerController, GeoPlayerState
```

## Key Files for Common Tasks
| Task | File |
|---|---|
| New single-shot ability | Extend `UGeoProjectileAbility` → `AbilitySystem/Abilities/Damaging/GeoProjectileAbility.h` |
| New hold-to-fire ability | Extend `UGeoAutomaticFireAbility`, override `ExecuteShot()` |
| New bullet pattern | Extend `UTickablePattern`, implement `TickPattern(ServerTime, SpentTime)` |
| New deployable | Extend `AGeoDeployableBase` → `Actor/Deployable/GeoDeployableBase.h` |
| Player attributes | `AbilitySystem/AttributeSet/CharacterAttributeSet.h` |
| Shared attributes | `AbilitySystem/AttributeSet/GeoAttributeSetBase.h` |
| Applying effects | `UGeoAbilitySystemLibrary::ApplyEffectFromEffectData()` |
| Projectile behavior | `Actor/Projectile/GeoProjectile.h` |
| New StateTree AI task | `AI/StateTree/` |
| HUD screen-space | `HUD/GeoHUD.h` (BlueprintImplementableEvent) |
| HUD world-space | WidgetComponent on character BP |
| Camera | `World/GeoGameCamera.h` |
| Movement speed | `Characters/Component/GeoCharacterMovementComponent.h` (`ApplySpeedMultiplier`) |
| Combat stats | `System/GeoCombatStatsSubsystem.h` |
