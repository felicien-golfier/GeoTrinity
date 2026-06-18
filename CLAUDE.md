# CLAUDE.md

## Project
GeoTrinity — multiplayer 2D boss fight bullet-hell, Unreal Engine 5.7, GAS. Players are geometric shapes (Tank=Square, Heal=Circle, DPS=Triangle). Camera: orthographic.

## Build
Use `AI/Commands.md` Bash build. Use MCP live compile only when actively working on Blueprints or code that directly touches MCP/Blueprint state.

## Big RULES
- **NEVER close, kill, or restart the user's Unreal editor — not even to build.** It may hold unsaved work that closing destroys. When a build needs the editor closed, ask the user to close it and wait; never do it yourself.
- ALWAYS read AI/CodingStyle.md before coding. If planing or just answering, don't, but if you write any line of code, read it.
- When opening a cpp file, read it entirely.
- ALWAYS open the public folder to the cpp file you are reading to have CLAUDE.md with the class explainations.
- Instead of creating a memory, place it in the relevant subfolder's CLAUDE.md.
- Don't spawn Explore agents to discover files before reading CLAUDE.md files — they already map every subsystem to its paths. Read the relevant CLAUDE.md (root + subfolder), then Read/Grep/Glob those files directly. Only spawn Explore for genuinely undocumented or ambiguous areas.
- ALWAYS read `AI/MCP/CLAUDE.md` before any MCP / Python editor automation task — it maps every topic to the right file.

## Key References
| Context | File |
|---|---|
| Any project's cpp file | CLAUDE.md at the .h root, in the public folder |
| Code style, GAS conventions, error handling | `AI/CodingStyle.md` |
| ALWAYS BUILD WHEN DONE CODING : Build commands, dev environment, copyright | `AI/Commands.md` |
| Networking, data structures, effect system | `AI/Architecture.md` |
| VFX / Niagara via MCP | `AI/VFX.md` |
| Any MCP / Python editor automation | `AI/MCP/CLAUDE.md` |
| AbilitySystem code | `Source/GeoTrinity/Public/AbilitySystem/CLAUDE.md` |
| Abilities (all classes) | `Source/GeoTrinity/Public/AbilitySystem/Abilities/CLAUDE.md` |
| Characters & components | `Source/GeoTrinity/Public/Characters/CLAUDE.md` |
| Actors (projectiles, deployables, turret) | `Source/GeoTrinity/Public/Actor/CLAUDE.md` |
| AI & StateTree | `Source/GeoTrinity/Public/AI/CLAUDE.md` |
| HUD & widgets (UI module) | `Source/GeoTrinityUI/Public/HUD/CLAUDE.md` |
| Gameplay↔UI interfaces (in gameplay module) | `Source/GeoTrinity/Public/HUD/Interface/` |
| Subsystems & pooling | `Source/GeoTrinity/Public/System/CLAUDE.md` |
| Editor automation utils (StateTree/Widget builders) | `Source/GeoTrinityEditor/Public/Tool/CLAUDE.md` |

## Source Structure (Every public folder has it's CLAUDE.md to explore)
```
Source/GeoTrinity/
├── Public/ & Private/
│   ├── AbilitySystem/
│   │   ├── Abilities/
│   │   │   ├── Base/          # GeoGameplayAbility, PatternAbility, AbilityPayload
│   │   │   ├── Damaging/      # GeoProjectileAbility, GeoAutomaticFireAbility, GeoAutomaticProjectileAbility
│   │   │   ├── Boss/          # GeoPeriodicFireAbility, GeoDevastatingWaveAbility
│   │   │   ├── Pattern/       # Pattern, SpiralPattern, SpawnPillarPattern, DevastatingWavePattern
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
│   │   ├── GeoEffectZone.h
│   │   └── GeoInteractableActor.h
│   ├── Characters/
│   │   ├── Component/         # GeoCharacterMovementComponent, GeoDeployableManagerComponent, GeoGameFeelComponent, ShieldBurstPassiveComponent, GeoBeamVFXComponent
│   │   ├── GeoCharacter.h
│   │   ├── PlayableCharacter.h
│   │   ├── EnemyCharacter.h
│   │   └── PlayerClassTypes.h # EPlayerClass enum
│   ├── AI/
│   │   ├── StateTree/
│   │   │   ├── Ability/       # STTask_FireAbility
│   │   │   ├── Blackboard/    # STTask_UpdateBlackboard
│   │   │   ├── Movement/      # STTask_MoveTo, GeoAITask_MoveTo, STTask_SelectNextFiringPoint
│   │   │   ├── Property/      # STPropertyFunction_GetHealthRatio, STPropertyFunction_GetBlackboard
│   │   │   └── Utility/       # STTask_SendEventAfterNCycles
│   │   ├── GeoAIBlackboardComponent.h
│   │   └── GeoEnemyAIController.h
│   ├── HUD/Interface/        # Gameplay↔UI seam (UINTERFACEs implemented by GeoTrinityUI widgets):
│   │                         #   GeoHUDInterface, GeoCombattantWidgetHost, GeoDeployGaugeWidgetInterface, GeoChargeBeamGaugeWidgetInterface
│   ├── Input/                 # GeoInputComponent
│   ├── Settings/              # GameDataSettings
│   ├── System/                # GeoActorPoolingSubsystem, GeoPoolableInterface, GeoCombatStatsSubsystem, GeoSessionSubsystem
│   ├── Tool/                  # UGeoGameplayLibrary, GeoAssetManager, Team
│   ├── World/                 # GeoGameCamera, GeoWorldSettings
│   ├── Animation/             # FireAnimNotify
│   └── GameClasses/           # GeoGameMode, GeoGameState, GeoGameInstance, GeoPlayerController, GeoPlayerState
└── (sibling modules)
Source/GeoTrinityUI/           # Runtime UI module (Type=Runtime) — all HUD/widgets/menus. Depends on GeoTrinity; gameplay
│                             # never depends on it (interface seam in GeoTrinity/Public/HUD/Interface). Excluded from the
│                             # dedicated-server target so the server package ships no Slate/UMG.
└── Public/ & Private/
    └── HUD/                  # Component/ (GeoCombattantWidgetComp), Menu/ (GeoMainMenuWidget, GeoBrowseServersWidget,
                              # GeoLocalConnectWidget, GeoCreateServerWidget, GeoServerRowWidget, GeoMenuButton), GeoHUD,
                              # GeoOverlayWidget, GeoAbilityBarWidget, GeoAbilitySlotWidget, GeoUserWidget,
                              # GenericCombattantWidget, GeoDeployChargeGaugeWidget, GeoChargeBeamGaugeWidget, HudFunctionLibrary
Source/GeoTrinityEditor/       # Editor-only module (Type=Editor) — UEditorUtilityObject automation utils kept out of packaged builds
└── Public/ & Private/
    └── Tool/                  # GeoStateTreeBuilderUtil, GeoWidgetBuilderUtil, GeoHudWidgetBuilderUtil
```
