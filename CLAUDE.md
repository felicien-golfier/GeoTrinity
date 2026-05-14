# CLAUDE.md

## Project
GeoTrinity — multiplayer 2D boss fight bullet-hell, Unreal Engine 5.7, GAS. Players are geometric shapes (Tank=Square, Heal=Circle, DPS=Triangle). Camera: orthographic.

## Build
Use `AI/Commands.md` Bash build. Use MCP live compile only when actively working on Blueprints or code that directly touches MCP/Blueprint state.

## Big RULES
- ALWAYS read AI/CodingStyle.md before coding. If planing or just answering, don't, but if you write any line of code, read it.
- When opening a cpp file, read it entirely.
- ALWAYS open the public folder to the cpp file you are reading to have CLAUDE.md with the class explainations.
- Instead of creating a memory, place it in the relevant subfolder's CLAUDE.md.
- Don't spawn Explore agents to discover files before reading CLAUDE.md files — they already map every subsystem to its paths. Read the relevant CLAUDE.md (root + subfolder), then Read/Grep/Glob those files directly. Only spawn Explore for genuinely undocumented or ambiguous areas.
- ALWAYS read `AI/MCP_Blueprint.md` before creating or modifying Blueprint assets via MCP Python scripts.

## Key References
| Context | File |
|---|---|
| Any project's cpp file | CLAUDE.md at the .h root, in the public folder |
| Code style, GAS conventions, error handling | `AI/CodingStyle.md` |
| ALWAYS BUILD WHEN DONE CODING : Build commands, dev environment, copyright | `AI/Commands.md` |
| Networking, data structures, effect system | `AI/Architecture.md` |
| VFX / Niagara via MCP | `AI/VFX.md` |
| Creating/configuring Blueprints via MCP Python | `AI/MCP_Blueprint.md` |
| AbilitySystem code | `Source/GeoTrinity/Public/AbilitySystem/CLAUDE.md` |
| Abilities (all classes) | `Source/GeoTrinity/Public/AbilitySystem/Abilities/CLAUDE.md` |
| Characters & components | `Source/GeoTrinity/Public/Characters/CLAUDE.md` |
| Actors (projectiles, deployables, turret) | `Source/GeoTrinity/Public/Actor/CLAUDE.md` |
| AI & StateTree | `Source/GeoTrinity/Public/AI/CLAUDE.md` |
| HUD & widgets | `Source/GeoTrinity/Public/HUD/CLAUDE.md` |
| Subsystems & pooling | `Source/GeoTrinity/Public/System/CLAUDE.md` |

## Source Structure (Every public folder has it's CLAUDE.md to explore)
```
Source/GeoTrinity/
├── Public/ & Private/
│   ├── AbilitySystem/
│   │   ├── Abilities/
│   │   │   ├── Base/          # GeoGameplayAbility, PatternAbility, AbilityPayload
│   │   │   ├── Damaging/      # GeoProjectileAbility, GeoAutomaticFireAbility, GeoAutomaticProjectileAbility
│   │   │   ├── Boss/          # GeoDelayedFatalZoneAbility
│   │   │   ├── Pattern/       # Pattern, SpiralPattern, FatalZonePattern
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
